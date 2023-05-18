#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#ifdef _WIN32
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


class EasyTcpServer {

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
			std::string data = "a new client joins, the socket is : " + _cSock;
			SendData2All(data.c_str());
			_client_sockets.push_back(_cSock);
			printf("<Socket=%d>accept success, the socket for connection from server is %d, the new client is %s\n", _sock, _cSock, inet_ntoa(clit_addr.sin_addr));
		}
		return _cSock;
	}

	//关闭socket系统资源
	void Close() {
		if (INVALID_SOCKET != _sock) {
#ifdef _WIN32
			//关闭资源
			for (auto fd : _client_sockets) {
				closesocket(fd);
			}
			closesocket(_sock);
			WSACleanup();
#else
			for (auto fd : _client_sockets) {
				close(fd);
			}
			close(_sock);
#endif
		}
	}

	bool OnRun() {
		if (IsRun()) {
			fd_set read_fds;
			fd_set write_fds;
			fd_set except_fds;

			FD_ZERO(&read_fds);
			FD_ZERO(&write_fds);
			FD_ZERO(&except_fds);

			//将服务器用于监听的文件描述符加入监听集合中
			FD_SET(_sock, &read_fds);
			FD_SET(_sock, &write_fds);
			FD_SET(_sock, &except_fds);

			//类Unix平台下，随着用于与客户端连接的文件描述符的增加，需要不断更新每次调用select时的第一个参数
			SOCKET max_sock = _sock;

			//将服务器用于和客户端通信的文件描述符加入监听集合中，描述符存放在g_clients
			for (auto fd : _client_sockets) {
				FD_SET(fd, &read_fds);
				if (max_sock < fd) {
					max_sock = fd;
				}
			}

			timeval val = { 0, 0 };
			if (select(max_sock + 1, &read_fds, &write_fds, &except_fds, &val) < 0) {
				printf("server select is over\n");
				//select结束了，一定要关闭系统资源socket
				Close();
				return false;
			}

			//deal new connect...
			if (FD_ISSET(_sock, &read_fds)) {
				FD_CLR(_sock, &read_fds);
				Accept();
			}

			//deal with send and recv...
			for (auto fd : _client_sockets) {
				if (FD_ISSET(fd, &read_fds)) {
					if (-1 == RecvData(fd)) {
						//如果客户端结束了连接，则客户端取消监听对应的文件描述符
						auto iter = std::find(_client_sockets.begin(), _client_sockets.end(), fd);
						if (iter != _client_sockets.end()) {
							_client_sockets.erase(iter);
						}
					}
				}
			}

			return true;
		}
		return false;
	}

	bool IsRun() {
		return INVALID_SOCKET != _sock;
	}

	int RecvData(SOCKET clit_sock) {
		char recvBuf[256] = {};
		int recvLen = recv(clit_sock, recvBuf, 256, 0);
		if (recvLen <= 0) {
			printf("the client terminated the connection...\n");
			return -1;
		}
		OnNetMessage(clit_sock, recvBuf);
		return 0;
	}

	//发送消息
	int SendData(SOCKET clit_sock, const char* data) {
		if (IsRun() && data) {
			return send(clit_sock, data, strlen(data) + 1, 0);
		}
		return INVALID_SOCKET;
	}

	void  SendData2All(const char* data) {
		if (IsRun() && data) {
			//向其他客户端广播消息
			for (auto fd : _client_sockets) {				
				SendData(fd, data);
			}
		}
	}

	virtual void OnNetMessage(SOCKET clit_sock, const char *data) {
		printf("receive the data from the client<socket=%d>\n", clit_sock);
		if (0 == strcmp(data, "getname")) {
			char sendBuf[] = "xiao ming";
			send(clit_sock, sendBuf, strlen(sendBuf) + 1, 0);
		}
		else if (0 == strcmp(data, "getage")) {
			char sendBuf[] = "99";
			send(clit_sock, sendBuf, strlen(sendBuf) + 1, 0);
		}
		else {
			char sendBuf[] = "unsupported command";
			send(clit_sock, sendBuf, strlen(sendBuf) + 1, 0);
		}
	}

private:
	SOCKET _sock;
	std::vector<SOCKET> _client_sockets;
};


#endif // !_EasyTcpServer_hpp_
