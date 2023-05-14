#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>

int main() {
	WORD ver = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(ver, &data);
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
	serv_addr.sin_addr.S_un.S_addr = INADDR_ANY;

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

	sockaddr_in clit_addr = {};
	int clit_addr_len = sizeof(clit_addr);
	while (true) {
		SOCKET _cSock = accept(_sock, (sockaddr*)&clit_addr, &clit_addr_len);
		if (INVALID_SOCKET == _cSock) {
			printf("accept error");
		}
		else {
			printf("accept success, the new client is %s\n", inet_ntoa(clit_addr.sin_addr));
		}

		char buf[] = "hello, i am the server";
		send(_cSock, buf, strlen(buf) + 1, 0);
	}
	closesocket(_sock);
	//-------------------
	WSACleanup();
	return 0;
}