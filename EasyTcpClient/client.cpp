#include <thread>

#include "easy_tcp_client.hpp"

bool g_bRun = true;

void CMD_Task() {
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
		else if (0 == strcmp(cmdBuf, "login")) {
			Login login("root", "root");
		}		
		else {
			printf("%s\n", cmdBuf);
			printf("unsupported command\n");
		}
	}
}
const int client_count = 10000;

const int thread_count = 4;

EasyTcpClient* clients[client_count];

void SendTask(int id) {
	
	int c = client_count / thread_count;

	int begin = (id - 1) * c;
	int end = id * c;

	for (int i = begin; i < end; i++) {		
		clients[i] = new EasyTcpClient();
	}

	for (int i = begin; i < end; i++) {
		clients[i]->Connect("127.0.0.1", 9527);
		printf("connect count %d\n", i);
	}

	Login login("root", "root");

	while (g_bRun)
	{
		for (int i = begin; i < end; i++) {
			clients[i]->SendData(&login);
		}
	}

	for (int i = begin; i < end; i++) {
		clients[i]->Close();
		delete clients[i];
	}
}

int main() {

	std::thread cmd_thread(CMD_Task);
	cmd_thread.detach();

	for (int i = 0; i < thread_count; i++) {
		std::thread t(SendTask, i + 1);
		t.detach();
	}
	
	while (1) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	
	return 0;
}
