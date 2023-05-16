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

std::vector<SOCKET> g_clients;

int processor(SOCKET conn_fd) {
	char recvBuf[256] = {};
	int recvLen = recv(conn_fd, recvBuf, 256, 0);
	if (recvLen <= 0) {
		printf("the client terminated the connection...\n");
		return -1;
	}
	printf("receive the data from the client<socket=%d>\n", conn_fd);
	if (0 == strcmp(recvBuf, "getname")) {
		char sendBuf[] = "xiao ming";
		send(conn_fd, sendBuf, strlen(sendBuf) + 1, 0);
	}
	else if (0 == strcmp(recvBuf, "getage")) {
		char sendBuf[] = "99";
		send(conn_fd, sendBuf, strlen(sendBuf) + 1, 0);
	}
	else {
		char sendBuf[] = "unsupported command";
		send(conn_fd, sendBuf, strlen(sendBuf) + 1, 0);
	}

	return 0;
}
int main() {
#ifdef _WIN32
	WORD ver = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(ver, &data);
#endif
	//---------------------
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == _sock) {
		printf("server create socket error\n");
	}
	else {
		printf("server create socket successfully\n");
	}
	sockaddr_in serv_addr = {};
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(9527);
#ifdef _WIN32
	serv_addr.sin_addr.S_un.S_addr = INADDR_ANY;
#else
	serv_addr.sin_addr.s_addr = INADDR_ANY;
#endif

	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&serv_addr, sizeof(serv_addr))) {
		printf("server bind socket error\n");
	}
	else {
		printf("server bind socket successfully\n");
	}

	if (SOCKET_ERROR == listen(_sock, 5)) {
		printf("server listen socket error\n");
	}
	else {
		printf("server listen socket successfully\n");
	}

	while (true) {
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
		for (auto fd : g_clients) {
			FD_SET(fd, &read_fds);
			if (max_sock < fd) {
				max_sock = fd;
			}
		}

		timeval val = { 0, 0 };
		if (select(max_sock + 1, &read_fds, &write_fds, &except_fds, &val) < 0) {
			printf("server select is over\n");
			break;
		}

		//deal new connect...
		if (FD_ISSET(_sock, &read_fds)) {
			FD_CLR(_sock, &read_fds);

			sockaddr_in clit_addr = {};
#ifdef _WIN32
			int clit_addr_len = sizeof(clit_addr);
			SOCKET _cSock = accept(_sock, (sockaddr*)&clit_addr, &clit_addr_len);
#else
			socklen_t clit_addr_len = sizeof(clit_addr);
			SOCKET _cSock = accept(_sock, (sockaddr*)&clit_addr, &clit_addr_len);
#endif
			//向其他客户端广播消息
			for (auto fd : g_clients) {
				std::string message = "a new client joins" + fd;
				send(fd, message.c_str(), strlen(message.c_str()) + 1, 0);
			}
			g_clients.push_back(_cSock);
			if (INVALID_SOCKET == _cSock) {
				printf("accept error");
			}
			else {
				printf("accept success, the socket is %d, the new client is %s\n", _cSock, inet_ntoa(clit_addr.sin_addr));
			}
		}

		//deal with send and recv...
		for (auto fd : g_clients) {
			if (FD_ISSET(fd, &read_fds)) {
				if (-1 == processor(fd)) {
					//如果客户端结束了连接，则客户端取消监听对应的文件描述符
					auto iter = std::find(g_clients.begin(), g_clients.end(), fd);
					if (iter != g_clients.end()) {
						g_clients.erase(iter);
					}
				}
			}
		}
	}
#ifdef _WIN32
	//关闭资源
	for (auto fd : g_clients) {
		closesocket(fd);
	}
	closesocket(_sock);
	WSACleanup();
#else
	for (auto fd : g_clients) {
		close(fd);
	}
	close(_sock);
#endif
	//-------------------
	return 0;
}