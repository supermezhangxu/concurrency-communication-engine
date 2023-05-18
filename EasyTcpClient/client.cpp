#include <thread>

#include "easy_tcp_client.hpp"


void CMD_Task(EasyTcpClient *client) {
	while (true) {
		char cmdBuf[256] = {};
#ifdef _WIN32
		scanf_s("%s", cmdBuf, 256);
#else 
		scanf("%s", cmdBuf);
#endif
		if (0 == strcmp(cmdBuf, "exit")) {
			printf("client is over...\n");
			client->Close();
			return;
		}
		else if (0 == strcmp(cmdBuf, "getname")) {
			char sendBuf[] = "getname";
			client->SendData(sendBuf);
		}
		else if (0 == strcmp(cmdBuf, "getage")) {
			char sendBuf[] = "getage";
			client->SendData(sendBuf);
		}
		else {
			printf("%s\n", cmdBuf);
			printf("unsupported command\n");
		}
	}
}

int main() {

	EasyTcpClient client;
	client.InitSocket();
	client.Connect("192.168.145.130", 9527);

	std::thread cmd_thread(CMD_Task, &client);
	cmd_thread.detach();

	while (client.IsRun())
	{
		client.OnRun();
	}

	client.Close();

	return 0;
}
