#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <iostream>
#include <signal.h>
#include <string.h>
#include <zlib.h>
#define SPORT 5001

using namespace std;

struct ServerStatus
{
	bool start = false;
	FILE* fp = 0;
	string filename;
	z_stream *p = 0;
	int recv_num = 0;
	int write_num = 0;
	~ServerStatus()
	{
		if(fp)
			fclose(fp);
		if(p)
			inflateEnd(p);
		p = 0;
		delete p;
	}
};


void event_cb(bufferevent* bev, short events, void* arg)
{
	cout << "server events_cb " << events << endl;
	ServerStatus* status = (ServerStatus*)arg;
	if(events & BEV_EVENT_EOF)
	{
		cout << "server event BEV_EVENT_EOF" << endl;
		if(status->fp)
		{
			fclose(status->fp);
			status->fp = 0;
		}
		bufferevent_free(bev);
		cout << "server recv "<< status->recv_num << " write " << status->write_num << endl;
	}
}


void read_cb(bufferevent* bev, void *arg)
{
//	cout << "[server R]" << flush;
	//回复ok
	ServerStatus* status = (ServerStatus*)arg;
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
//	cout << "server filter_in" << endl;
	ServerStatus* status = (ServerStatus*)arg;
	if(!status->start)
	{
		//接收客户端发送的文件名 然后回复ok
		char data[1024] = {0};
		int len = evbuffer_remove(s, data, sizeof(data) - 1);
		cout << "server recv ：" << data << endl;
		evbuffer_add(d, data, len);//将数据放入缓存
		return BEV_OK;
	}
#if 1
	//解压
	evbuffer_iovec v_in[1];
	//读取数据 不清理缓冲
	int n = evbuffer_peek(s, -1, NULL, v_in, 1);
	if(n <= 0)
		return  BEV_NEED_MORE;
	//解压上下文
	z_stream *p = status->p;

	//zlib 输入数据大小
	p->avail_in = v_in[0].iov_len;
	//zlib 输入数据地址
	p->next_in = (Byte*)v_in[0].iov_base;
	//申请输出空间大小
	evbuffer_iovec v_out[1];
	evbuffer_reserve_space(d, 4096, v_out, 1);
	//zlib 输出空间大小
	p->avail_out = v_out[0].iov_len;
	//zlib 输出空间地址
	p->next_out = (Byte*)v_out[0].iov_base;

	//zlib 解压缩
	int re = inflate(p, Z_SYNC_FLUSH);
	if(re != Z_OK)
	{
		cout << "defalate failed!!!!" << endl;
	}
	//解压用了多少数据，从source evbuffer中移除
	//p->avail_in   未处理数据大小
	int nread = v_in[0].iov_len - p->avail_in;

	//解压后数据大小 传入des evbuffer
	//p->avail_out   剩余空间大小
	int nwrite = v_out[0].iov_len - p->avail_out;

	//移除source evbuffer中的数据
	evbuffer_drain(s, nread);
	//传入des evbuffer
	v_out[0].iov_len = nwrite;
	evbuffer_commit_space(d, v_out, 1);
	cout << "sever nread = " << nread << "   nwrite = " << nwrite << endl;
	status->recv_num += nread;
	status->write_num += nwrite;
#endif
	return BEV_OK;
}


void listen_cb(struct evconnlistener* e, evutil_socket_t s, struct sockaddr* a, int socklen, void* arg)
{
	cout << "listen_cb " << endl;

	event_base* base = (event_base*)arg;
	
	//创建bufferevent 用于通信
	bufferevent* bev = bufferevent_socket_new(base, s, BEV_OPT_CLOSE_ON_FREE);
	ServerStatus* status = new ServerStatus();//堆区建立对象
	status->p = new z_stream();
	inflateInit(status->p);//压缩初始化

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


