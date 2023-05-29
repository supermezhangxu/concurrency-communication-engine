#include "easy_tcp_server.hpp"

bool g_bRun = true;

int main() {
	EasyTcpServer *server = new EasyTcpServer;

	server->InitSocket();
	server->Bind(nullptr, 9527);
	server->Listen(SOMAXCONN);
	server->Start();

	while (g_bRun) {
		server->OnRun();
	}
	server->Close();
	delete server;
	//-------------------
	return 0;
}