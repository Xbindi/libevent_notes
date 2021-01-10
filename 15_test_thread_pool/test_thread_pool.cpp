/***************************************************************************
 * 主要流程函数参见Xthread.h文件
 * 
 * 思路：
 * 		1.线程接收到连接后，生成任务对象XFtpServerCMD，传递sock，然后线程池分发线程
 * 		2.Dispatch分发函数根据轮训机制，找到空闲线程添加任务并激活线程，进入消息回调Notify
 * 		3.在Notify函数中读取消息，取出任务，根据添加线程时传递的base参数中包含的事件进行业务处理
 * 		4.在XFtpServerCMD::Init()实现自定义bufferevent任务的处理，为后续的ftp服务器开发准备
 * 测试：
 * 		使用多个终端telnet进行测试，发现线程池会逐个按顺序激活,10s自动退出
 * ************************************************************************/

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <iostream>
#include <signal.h>
#include <string.h>
#include "XThreadPool.h"
#include "XThread.h"
#include "XFtpServerCMD.h"
using namespace std;

#define SPORT 5001

void listen_cb(struct evconnlistener* e, evutil_socket_t s, struct sockaddr* a, int socklen, void* arg)
{
	cout << "listen_cb" << endl;
	XTask* task = new XFtpServerCMD();
	task->sock = s;
	XThreadPool::Get()->Dispatch(task);//分发线程
}

int main()
{
	//忽略管道信号，发送数据给以关闭的
	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
	{
		return 1;
	}

	std:: cout << "test thread poll!\n";
	//初始化线程池
	XThreadPool::Get()->Init(10);

	event_base* base = event_base_new();
	if(base)
	{
		cout << "event_base_new success!" << endl;
	}
	//设置监听的端口和地址
	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(SPORT);

	//监听端口
	evconnlistener* ev = evconnlistener_new_bind(base, //libevent的上下文
							listen_cb,	//接收到连接的回调函数
							base,		//回调函数获取的参数arg
							LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,	//地址重用，evconnlistener关闭同时关闭socket
							10,						//连接队列的大小，对应listen函数
							(sockaddr*)&sin, 		//绑定的地址和端口
							sizeof(sin));

	if(base)
		event_base_dispatch(base);
	if(ev)
		evconnlistener_free(ev);
	if(base)
		event_base_free(base);

	return 0;
}
