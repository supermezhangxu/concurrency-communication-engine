#include"easy_tcp_client.hpp"
#include"cell_timestamp.hpp"
#include<thread>
#include<atomic>

bool g_bRun = true;
void cmdThread()
{
	while (true)
	{
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit"))
		{
			g_bRun = false;
			break;
		}
		else {
			printf("��֧�ֵ����\n");
		}
	}
}


const int cCount = 8;

const int tCount = 4;

EasyTcpClient* client[cCount];
std::atomic_int sendCount = 0;
std::atomic_int readyCount = 0;

void sendThread(int id)
{
	printf("thread<%d>,start\n", id);
	int c = cCount / tCount;
	int begin = (id - 1) * c;
	int end = id * c;

	for (int n = begin; n < end; n++)
	{
		client[n] = new EasyTcpClient();
	}
	for (int n = begin; n < end; n++)
	{
		//win10 "192.168.1.110" i5 6300
		//win7 "192.168.1.114" i7 2670qm
		//127.0.0.1
		//39.108.13.69 
		client[n]->Connect("127.0.0.1", 9527);
	}

	printf("thread<%d>,Connect<begin=%d, end=%d>\n", id, begin, end);

	readyCount++;
	while (readyCount < tCount)
	{
		std::chrono::milliseconds t(10);
		std::this_thread::sleep_for(t);
	}

	Login login[10];
	for (int n = 0; n < 10; n++)
	{
		strcpy(login[n].userName, "lyd");
		strcpy(login[n].PassWord, "lydmm");
	}
	const int nLen = sizeof(login);
	while (g_bRun)
	{
		for (int n = begin; n < end; n++)
		{
			if (SOCKET_ERROR != client[n]->SendData(login, nLen))
			{
				sendCount++;
			}
			client[n]->OnRun();
		}
	}

	for (int n = begin; n < end; n++)
	{
		client[n]->Close();
		delete client[n];
	}

	printf("thread<%d>,exit\n", id);
}

int main()
{
	std::thread t1(cmdThread);
	t1.detach();

	for (int n = 0; n < tCount; n++)
	{
		std::thread t1(sendThread, n + 1);
		t1.detach();
	}

	CELLTimestamp tTime;

	while (g_bRun)
	{
		auto t = tTime.getElapsedSecond();
		if (t >= 1.0)
		{
			printf("thread<%d>,clients<%d>,time<%lf>,send<%d>\n", tCount, cCount, t, (int)(sendCount / t));
			sendCount = 0;
			tTime.update();
		}
		Sleep(1);
	}

	return 0;
}