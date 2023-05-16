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

bool g_bRun = true;

void CMD_Task(SOCKET conn_fd) {
	while (true) {
		char cmdBuf[256] = {};
#ifdef _WIN32
		scanf_s("%s", cmdBuf, 256);
#else 
		scanf("%s", cmdBuf);
#endif
		if (0 == strcmp(cmdBuf, "exit")) {
			g_bRun = false;
			printf("client is over...\n");
			return;
		}
		else if (0 == strcmp(cmdBuf, "getname")) {
			char sendBuf[] = "getname";
			send(conn_fd, sendBuf, strlen(sendBuf) + 1, 0);
		}
		else if (0 == strcmp(cmdBuf, "getage")) {
			char sendBuf[] = "getage";
			send(conn_fd, sendBuf, strlen(sendBuf) + 1, 0);
		}
		else {
			printf("%s\n", cmdBuf);
			printf("unsupported command\n");
			char sendBuf[] = "unsupported command";
			send(conn_fd, sendBuf, strlen(sendBuf) + 1, 0);
		}
	}
}

int processor(SOCKET conn_fd) {
	char buf[256] = {};
	int len = recv(conn_fd, buf, 256, 0);
	if (len > 0) {
		printf("recv data, the data is : %s\n", buf);
		return 0;
	}
	return -1;
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
		printf("client create socket error\n");
	}
	else {
		printf("client create socket success\n");
	}
	sockaddr_in serv_addr = {};
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(9527);
#ifdef _WIN32
	serv_addr.sin_addr.S_un.S_addr = inet_addr("192.168.145.1");
#else
	serv_addr.sin_addr.s_addr = inet_addr("192.168.145.1");
#endif
	if (SOCKET_ERROR == connect(_sock, (sockaddr*)&serv_addr, sizeof(serv_addr))) {
		printf("connect error");
	}
	else {
		printf("connect success");
	}

	std::thread cmd_thread(CMD_Task, _sock);
	cmd_thread.detach();

	while (g_bRun)
	{
		fd_set read_fds;

		FD_ZERO(&read_fds);

		FD_SET(_sock, &read_fds);
		timeval val = { 1, 0 };

		if (select(_sock + 1, &read_fds, NULL, NULL, &val) < 0) {
			printf("client select is over...\n");
			break;
		}

		if (FD_ISSET(_sock, &read_fds)) {
			FD_CLR(_sock, &read_fds);

			if (-1 == processor(_sock)) {
				printf("the connection occurs error...\n");
				break;
			}
		}
	}
#ifdef _WIN32	
	closesocket(_sock);
#else
	close(_sock);
#endif
	//---------------------
#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}
