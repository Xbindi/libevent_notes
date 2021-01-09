/*******************************************************
 * 测试方法：
 * 1. 使用makefile编译程序并执行
 * 2. 新开终端使用telnet 127.0.0.1 50001进行连接
 * 3. "ctrl+]"结束连接，q退出telnet，一次tcp连接结束
 * ***************************************************************/


#include <event2/event.h>
#include <event2/listener.h>
#include <iostream>
#include <signal.h>
#include <string.h>

using namespace std;

#define SPORT 50001
void listen_cb(struct evconnlistener* e, evutil_socket_t s, struct sockaddr* a, int socklen, void* arg)
{
	cout << "listen_cb " << endl;
}

int main()
{
	//忽略管道信号，发送数据给以关闭的
	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
	{
		return 1;
	}

	std:: cout << "test server!\n";

	event_base* base = event_base_new();
	if(base)
	{
		cout << "event_base_new success!" << endl;
	}
	
	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(SPORT);

	//监听端口
	evconnlistener* ev = evconnlistener_new_bind(base, //libevent的上下文
							listen_cb,	               //接收到连接的回调函数
							base,		               //回调函数获取的参数arg
							LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,	//地址重用，evconnlistener关闭同时关闭socket
							10,						   //连接队列的大小，对应listen函数
							(sockaddr*)&sin, 		   //绑定的地址和端口
							sizeof(sin));

	if(base)
		event_base_dispatch(base);
	if(ev)
		evconnlistener_free(ev);
	if(base)
		event_base_free(base);
	return 0;
}
