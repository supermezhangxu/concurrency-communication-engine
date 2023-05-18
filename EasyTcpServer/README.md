# c++�߲���ͨ������ʵ�� 

## �׶θ��¼�¼

### ����˺Ϳͻ��˲���selectģ��
select����ԭ�ͣ�linuxƽ̨�£�
~~~ 
int select(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout); 
~~~
����������
* nfds ��windows�¸ò���û�����壬������unixƽ̨�¸ò�����ʾҪ�������ļ��������������Ǹ���1
* readfds Ҫ�����Ķ��ļ���������Ҳ���ǿɶ�
* writefds Ҫ������д�ļ���������Ҳ���ǿ�д
* exceptfds Ҫ�������쳣������
* timeout ��ʱʱ�䣬��NULLΪ����ģʽ��Ҳ����io����ʱ���Ż᷵�ؼ���ִ�У�

���������selectģ�ͣ����������selectģ�ͽ�Ϊ�鷳����Ϊ�������ͻ���ͨѶʱ��������������ļ���������Ҳ��������socket��
�����Ҫ�������ļ������������뵽readfds����������Ϊ������select���������������ڼ������ļ��������ɶ�����ʱ��˵�����µĿͻ������ӣ�
��ʱ����accept�߼�����accept���ص�������ͻ���ͨѶ���ļ���������������С�������ͨѶ���ļ��������ɶ�����ʱ��˵���ͻ��˷������ݵ��������ˣ�
��ʱ���÷������Ĵ����߼������н��ռ��ɡ�

�ͻ�������selectģ�����������ƣ���Ϊ�򵥡�

### ����˺Ϳͻ�����ɿ�ƽ̨��ֲ��windows��linux��MacOS��
����ƽ̨�£���ͬ�ĵط��м����㣺
* ������ͷ�ļ��Ĳ�ͬ,�ڲ�ͬ��ƽ̨��ͨ��Ԥ��������������ɿ�ƽ̨
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
* һЩ�����Ĳ�ͬ�����纯��������

windowsƽ̨���е�
~~~
#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);
		WSADATA data;
		WSAStartup(ver, &data);
#endif
~~~
��ͬƽ̨�º����Ĳ�ͬ
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

### ����˺Ϳͻ�����ɷ�װ����ʼ�������

��c��cpp�������������Ľ׶Σ���һ�׶���Ҫ�ǽ�����˺Ϳͻ��˵Ĺ��ܽ��з�װ���Ա��ڸ��õ�ʹ�á�ͬʱ��
ҲΪ����������û������ڴ˻����ϣ�����Ϊ�������õ���չ���ܡ�





