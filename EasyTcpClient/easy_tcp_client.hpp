#ifndef _EasyTcpClient_
#define _EasyTcpClient_

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

#include "message_header.hpp"

class  EasyTcpClient
{
public:
	EasyTcpClient() {
		_sock = INVALID_SOCKET;
	}
	virtual ~EasyTcpClient() {
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
		/*if (INVALID_SOCKET == _sock) {
			printf("client create socket error\n");
		}
		else {
			printf("client create socket success\n");
		}*/
	}

	//与服务器进行连接
	int Connect(const char* ip, unsigned short port) {
		if (INVALID_SOCKET == _sock) {
			InitSocket();
		}
		sockaddr_in serv_addr = {};
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(port);
#ifdef _WIN32
		serv_addr.sin_addr.S_un.S_addr = inet_addr(ip);
#else
		serv_addr.sin_addr.s_addr = inet_addr(ip);
#endif
		int ret = connect(_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
		/*if (INVALID_SOCKET == ret) {
			printf("connect server error<Socket=%d>, server details:<ip=%s, port=%d>\n", _sock, ip, port);
		}
		else {
			printf("connect server success<Socket=%d>, server details:<ip=%s, port=%d>\n", _sock, ip, port);
		}*/
		return ret;
	}

	//关闭socke等系统资源
	void Close() {
		if (INVALID_SOCKET != _sock) {
#ifdef _WIN32	
			closesocket(_sock);
#else
			close(_sock);
#endif
			//---------------------
#ifdef _WIN32
			WSACleanup();
#endif
			_sock = INVALID_SOCKET;
		}
	}

	//启动监听
	bool OnRun() {
		if (IsRun()) {
			fd_set read_fds;

			FD_ZERO(&read_fds);

			FD_SET(_sock, &read_fds);
			timeval val = { 1, 0 };

			if (select(_sock + 1, &read_fds, NULL, NULL, &val) < 0) {
				printf("<Socket=%d>client select is over...\n", _sock);
				//这里select结束以后需要关闭socket，防止后续循环继续进入，重复select结束
				Close();
				return false;
			}

			//if (FD_ISSET(_sock, &read_fds)) {
			//	FD_CLR(_sock, &read_fds);

			//	if (-1 == RecvData()) {
			//		printf("<Socket=%d>the connection occurs error...\n", _sock);
			//		//这里select结束以后需要关闭socket，防止后续循环继续进入，重复select结束
			//		Close();
			//		return false;
			//	}
			//}
			return true;
		}
		return false;
	}

	bool IsRun() {
		return INVALID_SOCKET != _sock;
	}

#define RECV_BUF_SIZE 10240

	//接收缓冲区（第一个缓冲区）
	char _client_recv_buf[RECV_BUF_SIZE];
	//消息缓冲区，在这里进行处理消息
	char _client_msg_buf[RECV_BUF_SIZE * 10];
	int _lastPos = 0;
	//接收消息
	int RecvData() {
		int len = recv(_sock, _client_recv_buf, RECV_BUF_SIZE, 0);
		if (len <= 0) {
			printf("<Socket=%d>与服务器断开连接\n", _sock);
			return -1;
		}
		//每一次接收数据时，尽量将socket的接收缓冲区取完，保持通讯过程流畅
		std::memcpy(_client_msg_buf + _lastPos, _client_recv_buf, len);
		_lastPos += len;

		//当消息缓冲区收到 >= 一个数据包时，进行处理
		while (_lastPos >= sizeof(MessageHeader)) {
			MessageHeader* header = (MessageHeader*)_client_msg_buf;
			if (_lastPos >= header->_data_length) {
				int n_size = _lastPos - header->_data_length;
				OnNetMessage(header);
				std::memcpy(_client_recv_buf, _client_recv_buf + (header->_data_length), n_size);
				_lastPos = n_size;
			}
			else {
				break;
			}
		}
		return 0; 
	}

	//发送消息
	int SendData(MessageHeader *message) {
		if (IsRun() && message) {
			send(_sock, (const char *)message, message->_data_length, 0);
		}
		return INVALID_SOCKET;
	}

	//根据接受到的消息，进行响应
	virtual void OnNetMessage(MessageHeader *data) {
		switch (data->_message_type) {
		case MessageType::LOGINRESULT:
			LoginResult* login_result = (LoginResult*)data;
			//printf("<socket=%d>recv server's data, the data is : %s, the data length is %d\n", _sock, login_result->_login_result, login_result->_data_length);
			break;
		}
	}

private:
	SOCKET _sock;
};


#endif // !_EasyTcpClient_
