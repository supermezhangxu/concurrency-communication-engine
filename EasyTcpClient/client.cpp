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
		printf("client create socket error\n");
	}
	else {
		printf("client create socket success\n");
	}
	sockaddr_in serv_addr = {};
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(9527);
	serv_addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (SOCKET_ERROR == connect(_sock, (sockaddr*)&serv_addr, sizeof(serv_addr))) {
		printf("connect error");
	}
	else {
		printf("connect success");
	}
	char buf[256] = {};
	int len = recv(_sock, buf, 256, 0);
	if (len > 0) {
		printf("recv data, the data is : %s\n", buf);
	}
	closesocket(_sock);
	//---------------------
	WSACleanup();
	getchar();
	return 0;
}