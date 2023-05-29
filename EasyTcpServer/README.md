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

### ���tcp�ġ������ٰ������⣬�Լ����ⳡ����socket�������������������������

#### �����ٰ�

���ȣ����������ٰ�������tcp��������⣬��������ʹ���߶�tcp����ⲻ����������⣬
tcp���紫���д�������ޱ��޼ʵ��ֽ������޷����ϲ�Ӧ�ò�Э������ݰ��ֿ�����Ҫ�����ֶ����ú����ݰ��ı߽磬��ȡ���ݰ���
����취Ҳ�ܼ򵥣����ڱ䳤�����ݰ������ð�ͷ����ͷ��ָ�����ݰ��ĳ��ȣ������Ϳ��Խ�����������ˡ�

#### �������������������������

������������ԭ���кܶ࣬����һ�������ǵ����Ͷ˸�Ƶ���ٵķ������ݣ������ն˴����ٶȲ���ʱ��
��ʱtcp���ȡ�������ƻ��ƣ�Ҳ���ǽ��ն˻���߷��Ͷ˽��մ��ڵĴ�С��Ҳ����0����ʱ���Ͷ˲������ڷ������ݣ����Ƿ��Ͷ˴��������⣬
����һ��whileѭ����һֱ������ɻ�������������ݶ�ʧ��

����취��
����취Ҳ�ܼ򵥣����������ն˴������ݵ�Ч�ʡ������������ڽ��ն�ÿ�ε���recvʱ��Ҫ�����ܵĽ�socket�������е�����ȡ�ߣ���������һ����Ϣ��������
�����ݷŵ���Ϣ�������У�Ȼ���ٴ���Ϣ��������ȡ���ݡ������Ϳ��Ծ����ܵ�������ն˵Ĵ���������
����������£�
~~~
//������Ϣ
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
				//ȡ��һ�����ݰ��󣬻�������ʣ�����ݵĳ���
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

### �׶���СĿ�� 10000����ÿ�봦��300000�����ݰ�

��server�ļ������Ӻ�ά�����ӷֿ�����EasyTcpServer��ά��һ��CellServer���ϣ�
��������շ���Ϣ����CellServer��EasyTcpServer�������ӣ����ӽ����ɹ��󣬽�ClientSocket���������CellServer
���������ѡһ��ά�ֿͻ��������ٵ�CellServer��

#### ��ν�ClientSocket����CellServer��

����Ǿ����������������ģʽ��EasyTcpServer�������ߣ�CellServer�������ߣ�
CellServer�е�
~~~
	//�������Ŀͻ��˶���
	std::vector<ClientSocketRef> _client_socket_buf;
~~~
����Ϣ���ӵ�������EasyTcpServer������ţ�CellServer������ȡ���м���Ҫ��������ͬ����







