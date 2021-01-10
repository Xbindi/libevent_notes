#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <iostream>
#include <signal.h>
#include <string.h>
#include <zlib.h>
using namespace std;

#define SPORT 5001	

#define FILEPATH "log.txt"

struct ClientStatus
{
	FILE* fp = 0;
	bool end = false; 
	bool start_send = false;
	z_stream *z_output = 0;//压缩输出
	int read_num = 0;
	int send_num = 0;
	~ClientStatus()
	{
		if(fp)
			fclose(fp);
		fp = 0;
		if(z_output)
			deflateEnd(z_output);
		delete z_output;
		z_output = 0;
	}
};

void client_read_cb(bufferevent* bev, void *arg)
{
	ClientStatus *sta = (ClientStatus*)arg; 
	//cout << "[client R]\n" << flush;
	char data[1024] = {0};
	//002 读取接收服务器端返回的ok
	bufferevent_read(bev, data, sizeof(data) - 1);
	if(strcmp(data, "OK") == 0)
	{
		cout << data << endl;
		sta->start_send = true;//读到ok时开始发送数据
		//开始发送文件, 触发写入回调
		bufferevent_trigger(bev, EV_WRITE, 0);
	}
	else
	{
		bufferevent_free(bev);
	}
}


void client_write_cb(bufferevent* bev, void *arg)
{
	//cout << "[client W]" << flush;
	ClientStatus *cs = (ClientStatus*)arg; 
	FILE* fp = cs->fp;

	//判断什么时候清理资源
	if(cs->end)
	{
		//判断缓冲是否有数据，如果有则刷新
		//获取过滤器绑定的buffer
		bufferevent *be = bufferevent_get_underlying(bev);
		//获取输出缓冲及其大小
		evbuffer *evb = bufferevent_get_output(be);
		int len = evbuffer_get_length(evb);
		//cout << "evbuffer_get_length = " << len << endl;
		//如果缓冲无数据立刻清理buffer
		if(len <= 0)
		{
			cout << "client read "<< cs->read_num << " send " << cs->send_num << endl;
		    bufferevent_free(bev);
			delete cs;
			return;
		} 
		//如果缓冲有数据则刷新缓冲
		bufferevent_flush(bev, EV_WRITE, BEV_FINISHED);

		//立刻清理buffer，如果缓冲有数据则不会发送
		//bufferevent_free(bev);		
		return;
	}


	if (!fp) return ;
	//读取文件
	char data[1024] = {0};
	int len = fread(data, 1, sizeof(data), fp);
	if(len <= 0)
	{
		//文件读到结尾就关闭文件
		fclose(fp);//放入析构函数中处理
		cs->end = true;
		//刷新缓冲
	    bufferevent_flush(bev, EV_WRITE, BEV_FINISHED);
		return;
	}

	//发送文件
	bufferevent_write(bev, data, len);
}

bufferevent_filter_result filter_out(evbuffer* s, evbuffer* d, ev_ssize_t limit, 
					bufferevent_flush_mode mode, void* arg)
{
	//cout << "filter_out" << endl;
	ClientStatus *sta = (ClientStatus*)arg; 

	//压缩文件  发送文件消息001  去掉
	if(!sta->start_send) //如果没有发送数据
	{
		char data[1024] = {0};
		//将缓冲s取出来放入data中
		int len = evbuffer_remove(s, data, sizeof(data));
		evbuffer_add(d, data, len);
		return BEV_OK;
	}
#if 1
	//开始压缩文件
	//取出buffer中的数据引用
	evbuffer_iovec v_in[1];
	int n = evbuffer_peek(s, -1, 0, v_in, 1);
	
	if(n <= 0)
	{
		//调用write回调 清理空间
		if (sta->end)
			return BEV_OK;
		//没有数据  BEV_NEED_MORE 不会进入写入回调
		return BEV_NEED_MORE;
	}
	//zlib上下文
	z_stream *p = sta->z_output;
	if(!p)
	{
		return BEV_ERROR;
	}
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

	//zlib压缩  立刻刷新方式
	int re = deflate(p, Z_SYNC_FLUSH);
	if(re != Z_OK)
	{
		cout << "defalate failed!!!!" << endl;
	}
	//压缩用了多少数据，从source evbuffer中移除
	//p->avail_in 未处理数据大小
	int nread = v_in[0].iov_len - p->avail_in;

	//压缩后数据大小 传入des evbuffer
	//p->avail_out剩余空间大小
	int nwrite = v_out[0].iov_len - p->avail_out;

	//移除source evbuffer中的数据
	evbuffer_drain(s, nread);
	//传入des evbuffer
	v_out[0].iov_len = nwrite;
	evbuffer_commit_space(d, v_out, 1);

	cout << "client nread =" << nread << "   nwrite = " << nwrite << endl;
#endif
    sta->read_num += nread;
	sta->send_num += nwrite;
	return BEV_OK;
}

void client_event_cb(bufferevent* be, short events, void* arg)
{
	cout << "client_event_cb "<< events << endl;
	if(events & BEV_EVENT_CONNECTED) //连接成功事件
	{
		//001 发送文件名
		cout << "BEV_EVENT_CONNECTED" << endl;
		bufferevent_write(be, FILEPATH, strlen(FILEPATH));

		//打开文件，用client_read_cb读取信息
		FILE* fp = fopen(FILEPATH, "r");
		if(!fp)
		{
			perror("open file test1.txt failed!");
			cout << "open file " << FILEPATH << " failed!" << endl;
		}
		ClientStatus *cs = new ClientStatus();
		cs->fp = fp;

		//初始化zlib上下文 默认压缩方式
		cs ->z_output = new z_stream();
		deflateInit(cs->z_output, Z_DEFAULT_COMPRESSION);

		//添加输出过滤
		bufferevent* bev_filter = bufferevent_filter_new(be, 0, filter_out,
					BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS, 0, cs);

		//设置事件的写入、读出、事件回调函数 并设置权限
		bufferevent_setcb(bev_filter, client_read_cb, client_write_cb, client_event_cb, cs);
		bufferevent_enable(bev_filter, EV_READ | EV_WRITE);
	}
}


void Client(event_base* base)
{
	//链接服务器
	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(SPORT);
	evutil_inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr.s_addr);
	bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
	
	//只绑定事件回调， 用来确认链接成功
	bufferevent_enable(bev, EV_READ | EV_WRITE);
	bufferevent_setcb(bev, 0, 0, client_event_cb, 0);
	//建立连接
	bufferevent_socket_connect(bev, (sockaddr*)&sin, sizeof(sin));
	//接收回复确认ok
}
