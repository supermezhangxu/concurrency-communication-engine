#ifndef _EasyTcpClient_
#define _EasyTcpClient_

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


class  EasyTcpClient
{
public:
	EasyTcpClient() {
		_sock = INVALID_SOCKET;
	}
	virtual ~EasyTcpClient() {
		Close();
	}

	//��ʼ��socket
	void InitSocket() {
		if (INVALID_SOCKET != _sock) {
			printf("<Socket=%d>�����ӹرա�\n", _sock);
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
			printf("client create socket error\n");
		}
		else {
			printf("client create socket success\n");
		}
	}

	//���������������
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
		if (INVALID_SOCKET == ret) {
			printf("connect server error<Socket=%d>, server details:<ip=%s, port=%d>\n", _sock, ip, port);
		}
		else {
			printf("connect server success<Socket=%d>, server details:<ip=%s, port=%d>\n", _sock, ip, port);
		}
		return ret;
	}

	//�ر�socke��ϵͳ��Դ
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

	//��������
	bool OnRun() {
		if (IsRun()) {
			fd_set read_fds;

			FD_ZERO(&read_fds);

			FD_SET(_sock, &read_fds);
			timeval val = { 1, 0 };

			if (select(_sock + 1, &read_fds, NULL, NULL, &val) < 0) {
				printf("<Socket=%d>client select is over...\n", _sock);
				//����select�����Ժ���Ҫ�ر�socket����ֹ����ѭ���������룬�ظ�select����
				Close();
				return false;
			}

			if (FD_ISSET(_sock, &read_fds)) {
				FD_CLR(_sock, &read_fds);

				if (-1 == RecvData()) {
					printf("<Socket=%d>the connection occurs error...\n", _sock);
					//����select�����Ժ���Ҫ�ر�socket����ֹ����ѭ���������룬�ظ�select����
					Close();
					return false;
				}
			}
			return true;
		}
		return false;
	}

	bool IsRun() {
		return INVALID_SOCKET != _sock;
	}

	//������Ϣ
	int RecvData() {
		char buf[256] = {};
		int len = recv(_sock, buf, 256, 0);
		if (len > 0) {
			OnNetMessage(buf);
			return 0;
		}
		return -1;
	}

	//������Ϣ
	int SendData(const char *data) {
		if (IsRun() && data) {
			send(_sock, data, strlen(data) + 1, 0);
		}
		return INVALID_SOCKET;
	}

	//���ݽ��ܵ�����Ϣ��������Ӧ
	virtual void OnNetMessage(const char *data) {
		printf("recv server's data, the data is : %s\n", data);
	}

private:
	SOCKET _sock;
};


#endif // !_EasyTcpClient_
