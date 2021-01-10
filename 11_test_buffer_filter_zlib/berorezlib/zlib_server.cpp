#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <iostream>
#include <signal.h>
#include <string.h>

#define SPORT 5001

using namespace std;

struct Status
{
	bool start = false;
	FILE* fp = 0;
	string filename;
};


void event_cb(bufferevent* bev, short events, void* arg)
{
	cout << "events_cb" << events << endl;
	Status* status = (Status*)arg;
	if(events & BEV_EVENT_EOF)
	{
		cout << "server event BEV_EVENT_EOF" << endl;
		if(status->fp)
		{
			fclose(status->fp);
			status->fp = 0;
		}
		bufferevent_free(bev);
	}
}


void read_cb(bufferevent* bev, void *arg)
{
	cout << "[server R]" << flush;
	//回复ok
	Status* status = (Status*)arg;
	if(!status->start)
	{
		//001接收到文件名
		char data[1024] = {0};
		bufferevent_read(bev, data, sizeof(data) - 1);
		status->filename = data;

		string out = "out/";
		out += data;
		//打开写入文件
		status->fp = fopen(out.c_str(), "wb");
		if(!status->fp)
		{
			cout << "server open " << out << " failed!" << endl;
			return;
		}
		//002回复OK
		bufferevent_write(bev, "OK", 2);
		status->start = true;
		return;
	}
	do
	{
		//写入文件
	    char data[1024] = {0};
		int len = bufferevent_read(bev, data, sizeof(data));
		if(len >= 0)
		{
			fwrite(data, 1, len, status->fp);
			fflush(status->fp);
		}
	}while(evbuffer_get_length(bufferevent_get_input(bev)) > 0);//只要输入还有数据就读取

}


bufferevent_filter_result filter_in(evbuffer* s, evbuffer* d, ev_ssize_t limit, 
					bufferevent_flush_mode mode, void* arg)
{
	cout << "server filter_in" << endl;
	//接收客户端发送的文件名 然后回复ok
	char data[1024] = {0};
	int len = evbuffer_remove(s, data, sizeof(data) - 1);
	cout << "server recv data：" << data << endl;
	evbuffer_add(d, data, len);//将数据放入缓存 在read中回复ok
	
	return BEV_OK;
}




void listen_cb(struct evconnlistener* e, evutil_socket_t s, struct sockaddr* a, int socklen, void* arg)
{
	cout << "listen_cb " << endl;
	event_base* base = (event_base*)arg;
	
	//创建bufferevent 用于通信
	bufferevent* bev = bufferevent_socket_new(base, s, BEV_OPT_CLOSE_ON_FREE);
	Status* status = new Status();//堆区建立对象

	//绑定bufferevent filter 并设置回调函数
	bufferevent* bev_filter = bufferevent_filter_new(bev, 
								filter_in,  	//输入过滤函数
								0,		        //输出过滤函数
								BEV_OPT_CLOSE_ON_FREE,	//关闭filter同时关闭bufferevent
								0,				//清理的回调函数 
								status);				//传递给回调函数
	//设置bufferevent的回调  读取回调函数  事件处理回调（断开连接）
	bufferevent_setcb(bev_filter, read_cb, 0, event_cb, status);
	//设置过滤器权限
	bufferevent_enable(bev_filter, EV_READ|EV_WRITE);
}

void Server(event_base* base)
{
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
	
}


