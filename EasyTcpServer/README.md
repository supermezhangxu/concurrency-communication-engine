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





