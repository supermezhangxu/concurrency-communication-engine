#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#ifdef _WIN32
	#define FD_SETSIZE 10240
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include <windows.h>
	#include <WinSock2.h>
#else
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <string.h>

	#define SOCKET int
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif

#include <stdio.h>
#include <thread>
#include <vector>
#include <string>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <functional>

#include "message_header.hpp"
#include "cell_timestamp.hpp"

#define CELL_SERVER_COUNT 4
#define RECV_BUF_SIZE 10240

class ClientSocket {
private:
	SOCKET _client_sock;
	char _recv_buf[RECV_BUF_SIZE * 10];
	int _lastPos;
public:
	ClientSocket(SOCKET socket = INVALID_SOCKET) : _client_sock(socket), _lastPos(0){
		std::memset(_recv_buf, 0, sizeof(_recv_buf));
	}

	SOCKET GetSocketFd() {
		return _client_sock;
	}

	char* GetRecvBuf() {
		return _recv_buf;
	}

	int GetLastPos() {
		return _lastPos;
	}

	void SetLastPos(int pos) {
		_lastPos = pos;
	}
};

class INetEvent {
	using ClientSocketRef = std::shared_ptr<ClientSocket>;
public:
	virtual void OnLeave(ClientSocketRef client) = 0;
};

//用来与客户端收发消息的客户端
class CellServer {
	using ClientSocketRef = std::shared_ptr<ClientSocket>;
public:
	CellServer(SOCKET socket, INetEvent *event) : _sock(socket), _recv_count(0), _event(event){}

	virtual ~CellServer() {
		_sock = INVALID_SOCKET;
		Close();
	}

	void OnRun() {
		while (IsRun()) {
			fd_set read_fds;

			FD_ZERO(&read_fds);

			//从监听客户端缓冲队列中取出要监听的clientsocket
			if (!_client_socket_buf.empty()) {
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto client : _client_socket_buf) {
					_client_sockets.push_back(client);
				}
				//取完之后，清空缓冲队列
				_client_socket_buf.clear();
			}

			if (_client_sockets.empty()) {
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			//类Unix平台下，随着用于与客户端连接的文件描述符的增加，需要不断更新每次调用select时的第一个参数
			SOCKET max_sock = _client_sockets[0]->GetSocketFd();

			//将服务器用于和客户端通信的文件描述符加入监听集合中，描述符存放在g_clients
			for (auto client : _client_sockets) {
				FD_SET(client->GetSocketFd(), &read_fds);
				if (max_sock < client->GetSocketFd()) {
					max_sock = client->GetSocketFd();
				}
			}

			//timeval val = { 1, 0 };
			if (select(max_sock + 1, &read_fds, NULL, NULL, NULL) < 0) {
				printf("server select is over\n");
				//select结束了，一定要关闭系统资源socket
				Close();
				return;
			}

			//deal with send and recv...
			//代码写法有问题，在循环过程中，删除元素会导致迭代器失效
			//for (auto client : _client_sockets) {
			//	if (client) {
			//		if (FD_ISSET(client->GetSocketFd(), &read_fds)) {
			//			if (-1 == RecvData(client)) {
			//				//如果客户端结束了连接，则客户端取消监听对应的文件描述符
			//				auto iter = std::find(_client_sockets.begin(), _client_sockets.end(), client);
			//				if (iter != _client_sockets.end()) {
			//					//这里需要释放client缓冲区资源
			//					_client_sockets.erase(iter);
			//					_event->OnLeave(client);
			//				}
			//			}
			//		}
			//	}
			//}

			//for (int i = (int)_client_sockets.size() - 1; i >= 0; i--) {
			//	if (FD_ISSET(_client_sockets[i]->GetSocketFd(), &read_fds)) {
			//		if (-1 == RecvData(_client_sockets[i])) {						
			//			//如果客户端结束了连接，则将其放入待删除的列表中
			//			auto iter = _client_sockets.begin() + i;
			//			if (iter != _client_sockets.end()) {
			//				_event->OnLeave(_client_sockets[i]);
			//				_client_sockets.erase(iter);
			//			}						
			//		}
			//	}
			//}

			std::vector<ClientSocketRef> detete_clients;
			for (auto& client : _client_sockets) {				
				if (FD_ISSET(client->GetSocketFd(), &read_fds)) {
					if (-1 == RecvData(client)) {
						//如果客户端结束了连接，则将其放入待删除的列表中
						detete_clients.push_back(client);
						_event->OnLeave(client);
					}
				}
			}

			// 删除所有需要删除的元素
			_client_sockets.erase(std::remove_if(_client_sockets.begin(), _client_sockets.end(),
				[&](auto& ptr) {
					return std::find(detete_clients.begin(), detete_clients.end(), ptr) != detete_clients.end();
				}),
				_client_sockets.end());		
		}
	}

	bool IsRun() {
		return INVALID_SOCKET != _sock;
	}

	//接收消息
	int RecvData(ClientSocketRef client_socket) {
		int recv_len = recv(client_socket->GetSocketFd(), _server_recv_buf, RECV_BUF_SIZE, 0);
		if (recv_len <= 0) {
			printf("the client terminated the connection...\n");
			return -1;
		}

		char* client_recv_buf = client_socket->GetRecvBuf();
		int lastpos = client_socket->GetLastPos();
		std::memcpy(client_recv_buf + lastpos, _server_recv_buf, recv_len);
		client_socket->SetLastPos(lastpos + recv_len);

		while (client_socket->GetLastPos() >= sizeof(MessageHeader)) {
			MessageHeader* header = (MessageHeader*)client_recv_buf;
			if (client_socket->GetLastPos() >= header->_data_length) {
				//取出一个数据包后，缓冲区中剩余数据的长度
				auto n_size = client_socket->GetLastPos() - header->_data_length;
				OnNetMessage(client_socket, header);
				std::memcpy(client_socket->GetRecvBuf(), client_socket->GetRecvBuf() + (header->_data_length), n_size);
				client_socket->SetLastPos(n_size);
			}
			else {
				break;
			}
		}
		return 0;
	}

	//发送消息
	int SendData(SOCKET clit_sock, MessageHeader* data) {
		if (IsRun() && data) {
			return send(clit_sock, (const char*)data, data->_data_length, 0);
		}
		return INVALID_SOCKET;
	}

	void  SendData2All(MessageHeader* data) {
		if (IsRun() && data) {
			//向其他客户端广播消息
			for (auto client : _client_sockets) {
				SendData(client->GetSocketFd(), data);
			}
		}
	}

	virtual void OnNetMessage(ClientSocketRef client, MessageHeader* message) {
		_recv_count++;
		switch (message->_message_type)
		{
		case MessageType::LOGIN:
			//LoginResult result("login result is successful");
			//SendData(client->GetSocketFd(), &result);
			break;
		}
	}

	void AddClient2Buf(ClientSocketRef client) {
		std::lock_guard<std::mutex> lock(_mutex);
		_client_socket_buf.push_back(client);
	}

	//关闭socket系统资源
	void Close() {
		if (INVALID_SOCKET != _sock) {
#ifdef _WIN32
			//关闭资源
			for (auto client : _client_sockets) {
				closesocket(client->GetSocketFd());
			}
			delete _thread;
			WSACleanup();
#else
			for (auto client : _client_sockets) {
				close(client->GetSocketFd());
			}
			delete _thread;
#endif
		}
	}

	void Start() {
		_thread = new std::thread(std::mem_fn(&CellServer::OnRun), this);
	}

	size_t GetClientCount() {
		return _client_sockets.size() + _client_socket_buf.size();
	}

private:
	SOCKET _sock;
	std::vector<ClientSocketRef> _client_sockets;
	//待监听的客户端队列
	std::vector<ClientSocketRef> _client_socket_buf;
	std::mutex _mutex;
	std::thread* _thread;
	//第一缓冲区，过渡的缓冲区
	char _server_recv_buf[RECV_BUF_SIZE];
	INetEvent* _event;
public:
	std::atomic_int _recv_count;
};

//用来监听客户端连接请求的服务器
class EasyTcpServer : public INetEvent{
	using ClientSocketRef = std::shared_ptr<ClientSocket>;
public:
	EasyTcpServer() {
		_sock = INVALID_SOCKET;
	}

	virtual ~EasyTcpServer() {
		Close();
	}

	//初始化socket
	void InitSocket() {
		if (INVALID_SOCKET != _sock) {
			printf("<Socket=%d>旧连接关闭。\n", _sock);
			Close();
		}
#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);
		WSADATA data;
		WSAStartup(ver, &data);
#endif
		//---------------------
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock) {
			printf("server create socket error, socket = %d\n", _sock);
		}
		else {
			printf("server create socket success, socket = %d\n", _sock);
		}
	}

	//服务器绑定socket
	int Bind(const char *ip, unsigned short port) {
		if (INVALID_SOCKET == _sock) {
			InitSocket();
		}
		sockaddr_in serv_addr = {};
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(port);
#ifdef _WIN32
		if (ip) {
			serv_addr.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else {
			serv_addr.sin_addr.S_un.S_addr = INADDR_ANY;
		}	
#else
		if (ip) {
			serv_addr.sin_addr.s_addr = inet_addr(ip);
		}
		else {
			serv_addr.sin_addr.s_addr = INADDR_ANY;
		}
#endif
		int ret = bind(_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
		if (SOCKET_ERROR == ret) {
			printf("server bind socket error, socket bind <ip = %s, port = %d>\n", ip, port);
		}
		else {
			if (ip) {
				printf("server bind socket successfully, socket bind <ip = %s, port = %d>\n", ip, port);
			}
			else {
				printf("server bind socket successfully, socket bind <ip = %s, port = %d>\n", "127.0.0.1", port);
			}			
		}
		return ret;
	}

	//监听socket
	int Listen(int backlog) {
		if (INVALID_SOCKET == _sock) {
			InitSocket();
		}
		int ret = listen(_sock, backlog);
		if (SOCKET_ERROR == ret) {
			printf("server listen socket error, socket = %d\n", _sock);
		}
		else {
			printf("server listen socket successfully, socket = %d\n", _sock);
		}
		return ret;
	}

	//监听连接
	SOCKET Accept() {
		sockaddr_in clit_addr = {};
#ifdef _WIN32
		int clit_addr_len = sizeof(clit_addr);
		SOCKET _cSock = accept(_sock, (sockaddr*)&clit_addr, &clit_addr_len);
#else
		socklen_t clit_addr_len = sizeof(clit_addr);
		SOCKET _cSock = accept(_sock, (sockaddr*)&clit_addr, &clit_addr_len);
#endif		
		if (INVALID_SOCKET == _cSock) {
			printf("<Socket=%d>accept error\n", _sock);
		}
		else {
			//向全体客户端广播消息
			//std::string data = "a new client joins, the socket is : " + _cSock;
			//SendData2All(data.c_str());
			//_client_sockets.push_back(std::make_shared<ClientSocket>(_cSock));
			AddClientToCellServer(std::make_shared<ClientSocket>(_cSock));
			//printf("<Socket=%d>accept success, the socket for connection from server is %d, the new client is %s\n", _sock, _cSock, inet_ntoa(clit_addr.sin_addr));
		}
		return _cSock;
	}

	void AddClientToCellServer(ClientSocketRef client) {
		_clients.push_back(client);
		auto server = _cell_servers[0];
		for (auto cell_server : _cell_servers) {
			if (cell_server->GetClientCount() < server->GetClientCount()) {
				server = cell_server;
			}
		}
		server->AddClient2Buf(client);
	}

	//关闭socket系统资源
	void Close() {
		if (INVALID_SOCKET != _sock) {
#ifdef _WIN32
			//关闭资源
			for (auto server : _cell_servers) {
				server->Close();
				delete server;
			}
			closesocket(_sock);
			WSACleanup();
#else
			for (auto server : _cell_servers) {
				server->Close();
				delete server;
			}
			close(_sock);
#endif
		}
	}

	bool OnRun() {
		if (IsRun()) {

			Time4Msg();

			fd_set read_fds;

			FD_ZERO(&read_fds);

			//将服务器用于监听的文件描述符加入监听集合中
			FD_SET(_sock, &read_fds);

			timeval val = { 0, 10 };
			//参数timeout传NULL，则一直阻塞直到有描述符就绪
			if (select(_sock + 1, &read_fds, NULL, NULL, &val) < 0) {
				printf("server select is over\n");
				//select结束了，一定要关闭系统资源socket
				Close();
				return false;
			}

			//deal new connect...
			if (FD_ISSET(_sock, &read_fds)) {
				FD_CLR(_sock, &read_fds);
				Accept();
				return true;
			}

			return true;
		}
		return false;
	}

	bool IsRun() {
		return INVALID_SOCKET != _sock;
	}

	void Time4Msg() {
		
		//统计每秒收到的包的数量
		double t = _time.GetElapsedTimeInSecond();
		if (t > 1.0) {
			int recv_count = 0;
			for (auto server : _cell_servers) {
				recv_count += server->_recv_count;
				server->_recv_count = 0;
			}
			printf("time<%lf>, socket<%d>, clients<%d>, recvCount<%d>\n", t, _sock, _clients.size(), (int)(recv_count / t));
			_time.update();
		}
	}

	void Start() {
		for (int i = 0; i < CELL_SERVER_COUNT; i++) {
			auto cell_server = new CellServer(_sock, this);
			_cell_servers.push_back(cell_server);
			cell_server->Start();
		}
	}

	virtual void OnLeave(ClientSocketRef client) {
		std::lock_guard<std::mutex> guard(_mutex);
		for (int i = (int)_clients.size() - 1; i >= 0; i--) {
			if (_clients[i] == client) {
				auto iter = _clients.begin() + i;
				_clients.erase(iter);
			}
		}
	}
	
private:
	SOCKET _sock;
	std::vector<ClientSocketRef> _clients;
	std::vector<CellServer*> _cell_servers;
	CELLTimestamp _time;
	std::mutex _mutex;
};


#endif // !_EasyTcpServer_hpp_
