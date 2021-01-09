/********************************************************************************
 * 缓冲IO evbuffer 
 * 读取回调、写入回调  通过水位调节读写操作
 * 低水位保证了程序性能不会被碎片数据影响，数据量达到一定数值才被处理（类似于硬盘写入大量小文件速率会降低）
 * 高水位保证了程序最大处理的数据量，当大于高水位数值时会被分成多次处理
 * 
 * 读取操作使得输入缓冲区的数据量达到或高于此级别时（读取低水位），读取回调将被调用，既小于读取低水位时，读取回调不再被使用；
 * 大于等于读取高水位时停止读取，直到输入缓冲区中足够的数据被抽取，使得数据低于此级别
 * 
 * 写入操作使得输出缓冲区的数据量达到或低于此级别时（写入低水位），写入回调将被调用
 * *********************************************************************************/

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <iostream>
#include <signal.h>
#include <string.h>

using namespace std;

#define SPORT 5001
//异常回调函数，当错误、超时（连接断开时会进入）
void event_cb(bufferevent* be, short events, void *arg)
{
	cout << "[E]" << flush;
	//超时事件发生时，读取事件停止
	if(events & BEV_EVENT_TIMEOUT && events & BEV_EVENT_READING)
	{
		cout << "BEV_EVENT_TIMEOUT BEV_EVENT_READING" << endl;
	//	bufferevent_enable(be, EV_READ);   //再次开启读事件
		bufferevent_free(be);			//超时清理掉链接
	}
	else if(events & BEV_EVENT_ERROR)
	{
		cout << "BEV_EVENT_ERROR" << endl;
		bufferevent_free(be);	
	}
	else
	{
		cout << "OTHERS" << endl;
	}
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
	cout << "========>" << data << endl;
	if(len <= 0) return ;
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
							5,		//低水位, 默认是0为无限制
						   	10);		//高水位，0为无限制

	//写水位
	bufferevent_setwatermark(bev, EV_WRITE, 
							5,		//低水位, 默认是0, 缓冲数据低于5才会调用写入回调函数
						   	0); 	//高水位暂时无效
	//超时时间设置
	timeval t1 = {5, 0};
	bufferevent_set_timeouts(bev, &t1, 0);

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
