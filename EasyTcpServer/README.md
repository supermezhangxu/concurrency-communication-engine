# c++高并发通信引擎实现 

## 阶段更新记录

### 服务端和客户端采用select模型
select函数原型（linux平台下）
~~~ 
int select(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout); 
~~~
参数解析：
* nfds 在windows下该参数没有意义，而在类unix平台下该参数表示要监听的文件描述符中最大的那个加1
* readfds 要监听的读文件描述符，也就是可读
* writefds 要监听的写文件描述符，也就是可写
* exceptfds 要监听的异常描述符
* timeout 超时时间，传NULL为阻塞模式，也就是io就绪时，才会返回继续执行；

服务端升级select模型：服务端升级select模型较为麻烦，因为服务端与客户端通讯时，服务端是两个文件描述符，也就是两个socket，
因此需要将两个文件描述符都加入到readfds。大致流程为：调用select启动监听，当用于监听的文件描述符可读就绪时，说明有新的客户端连接，
这时进入accept逻辑，将accept返回的用于与客户端通讯的文件描述符加入监听中。当用于通讯的文件描述符可读就绪时，说明客户端发送内容到服务器了，
这时调用服务器的处理逻辑，进行接收即可。

客户端升级select模型与服务端类似，较为简单。

### 服务端和客户端完成跨平台移植（windows，linux，MacOS）
各个平台下，不同的地方有几个点：
* 首先是头文件的不同,在不同的平台下通过预处理器处理来完成跨平台
~~~
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
~~~
* 一些函数的不同，例如函数参数等

windows平台特有的
~~~
#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);
		WSADATA data;
		WSAStartup(ver, &data);
#endif
~~~
不同平台下函数的不同
~~~
#ifdef _WIN32
		if (ip) {
			serv_addr.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else {
			serv_addr.sin_addr.S_un.S_addr = INADDR_ANY;
		}	
#else
		if (ip) {
			serv_addr.sin_addr.s_addr = inet_addr(ip);
		}
		else {
			serv_addr.sin_addr.s_addr = INADDR_ANY;
		}
#endif
~~~

### 服务端和客户端完成封装，开始面向对象

从c到cpp，进入面向对象的阶段，这一阶段主要是将服务端和客户端的功能进行封装，以便于更好的使用。同时，
也为后续开发打好基础，在此基础上，可以为后续更好的扩展功能。

### 解决tcp的“黏包，少包”问题，以及特殊场景下socket缓冲区溢出，网络阻塞等问题

#### 黏包，少包

首先，上述黏包和少包都不是tcp本身的问题，而是由于使用者对tcp的理解不够引起的问题，
tcp网络传输中传输的是无边无际的字节流，无法将上层应用层协议的数据包分开，需要我们手动设置好数据包的边界，提取数据包。
解决办法也很简单，对于变长的数据包，设置包头，包头内指明数据包的长度，这样就可以解决上述问题了。

#### 缓冲区溢出，网络阻塞等问题

造成上述问题的原因有很多，其中一个场景是当发送端高频快速的发送数据，而接收端处理速度不够时，
这时tcp会采取流量控制机制，也就是接收端会告诉发送端接收窗口的大小，也就是0，这时发送端不可以在发送数据，但是发送端代码有问题，
就是一个while循环，一直发，造成缓冲区溢出，数据丢失。

解决办法：
解决办法也很简单，即提升接收端处理数据的效率。具体做法是在接收端每次调用recv时，要尽可能的将socket缓冲区中的数据取走，可以设置一个消息缓冲区，
将数据放到消息缓冲区中，然后再从消息缓冲区中取数据。这样就可以尽可能的增大接收端的处理能力。
具体代码如下：
~~~
//接收消息
	int RecvData(ClientSocketRef client_socket) {
		int recv_len = recv(client_socket->GetSocketFd(), _server_recv_buf, RECV_BUF_SIZE, 0);
		if (recv_len <= 0) {
			printf("the client terminated the connection...\n");
			return -1;
		}

		char* client_recv_buf = client_socket->GetRecvBuf();
		int lastpos = client_socket->GetLastPos();
		std::memcpy(client_recv_buf + lastpos, _server_recv_buf, recv_len);
		client_socket->SetLastPos(lastpos + recv_len);

		while (client_socket->GetLastPos() >= sizeof(MessageHeader)) {
			MessageHeader* header = (MessageHeader*)client_recv_buf;
			if (client_socket->GetLastPos() >= header->_data_length) {
				//取出一个数据包后，缓冲区中剩余数据的长度
				auto n_size = client_socket->GetLastPos() - header->_data_length;
				OnNetMessage(client_socket, header);
				std::memcpy(client_socket->GetRecvBuf(), client_socket->GetRecvBuf() + (header->_data_length), n_size);
				client_socket->SetLastPos(n_size);
			}
			else {
				break;
			}
		}
		return 0;
	}
~~~

### 阶段性小目标 10000连接每秒处理300000个数据包

将server的监听连接和维持连接分开，在EasyTcpServer中维持一个CellServer集合，
将具体的收发消息交给CellServer。EasyTcpServer监听连接，连接建立成功后，将ClientSocket交给具体的CellServer
，这里会挑选一个维持客户端数量少的CellServer。

#### 如何将ClientSocket交给CellServer？

这就是经典的生产者消费者模式，EasyTcpServer是生产者，CellServer是消费者，
CellServer中的
~~~
	//待监听的客户端队列
	std::vector<ClientSocketRef> _client_socket_buf;
~~~
是消息连接的桥梁，EasyTcpServer往里面放，CellServer从里面取，中间需要加锁保持同步。







