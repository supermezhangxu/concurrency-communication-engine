#include "easy_tcp_server.hpp"

int main() {
	EasyTcpServer server;

	server.InitSocket();
	server.Bind(nullptr, 9527);
	server.Listen(5);

	while (server.IsRun()) {
		server.OnRun();
	}
	server.Close();
	//-------------------
	return 0;
}