#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <iostream>
#include <signal.h>
#include <string.h>

using namespace std;

#define SPORT 5001

void event_cb(bufferevent* be, short events, void *arg)
{
	cout << "[E]" << flush;
}


void write_cb(bufferevent* be, void *arg)
{
	cout << "[W]" << flush;
}

void read_cb(bufferevent* be, void *arg)
{
	cout << "[R]" << flush;
	char data[1024] = {0};
	//读取缓冲数据并且发送
	int len = bufferevent_read(be, data, sizeof(data) - 1);
	cout << "["<< data <<"]" << endl;
	if(len <= 0) return;
	if(strstr(data, "quit") != NULL)
	{
		cout << "quit" << endl;
		//退出并关闭socket
		bufferevent_free(be);
	}
	bufferevent_write(be, "ok", 3);
}


void listen_cb(struct evconnlistener* e, evutil_socket_t s, struct sockaddr* a, int socklen, void* arg)
{
	cout << "listen_cb" << endl;

	event_base* base = (event_base*)arg;
	//创建bufferevent上下文， BEV_OPT_CLOSE_ON_FREE清理bufferevent时关闭socket
	bufferevent* bev = bufferevent_socket_new(base, s, BEV_OPT_CLOSE_ON_FREE);
	//添加监控事件
	bufferevent_enable(bev, EV_READ | EV_WRITE);

	//设置水位
	//读取水位
	bufferevent_setwatermark(bev, EV_READ, 
							10,		//低水位, 默认是0
						   	20);		//高水位，0为无限制

	//设置回调函数
	bufferevent_setcb(bev, read_cb, write_cb, event_cb, base);
}


int main()
{
	//忽略管道信号，发送数据给以关闭的
	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
	{
		return 1;
	}

	std:: cout << "test bufferevent server!\n";
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
