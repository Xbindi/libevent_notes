#include "XFtpServerCMD.h"
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <iostream>
#include <string>
using namespace std;


void XFtpServerCMD::Reg(std::string cmd, XFtpTask* call)
{
	if(!call)
	{
		cout << "XFtpServerCMD::Reg call is null " << endl;
		return ;
	}

	if(cmd.empty())
	{
		cout << "XFtpServerCMD:: Reg cmd is null" << endl;
		return ;	
	}

	//已经注册的是否覆盖，不覆盖，提示错误
	if(calls.find(cmd) != calls.end())
	{
		cout << cmd << " is already register" << endl;
		return ;
	}
	calls[cmd] = call;
	calls_del[call] = 0;//用于空间清理
}


void XFtpServerCMD::Read(struct bufferevent* bev)
{
	char data[1024] = {0};
	for(;;)
	{
		int len = bufferevent_read(bev, data, sizeof(data) - 1);
		if(len <= 0) break;
		data[len] = '\0';
		cout << "recv CMD:" << data << flush;
		//分析出类型
		string type = "";
		for(int i = 0; i < len; i++)
		{
			if(data[i] == ' ' || data[i] == '\r')
				break;
			type += data[i];
		}
	
		cout << "type is [" << type << "]"<< endl;
		//分发到处理对象
		if(calls.find(type) != calls.end())
		{
			//命令找到了 就解析命令并处理
			XFtpTask* t = calls[type];
			t->cmdTask = this;  //用来处理回复命令和目录
			t->ip = ip; //保存ip和端口
			t->port = port;
			t->base =  base;
			t->Parse(type, data);
			if(type == "PORT")
			{
				ip = t->ip;
				port = t->port;
			}
		}
		else
		{
			//没找到就回复200
			string msg = "200 OK\r\n";
			bufferevent_write(bev, msg.c_str(), msg.size());
		}	
	}
}
void XFtpServerCMD::Event(struct bufferevent* bev, short what)
{

	//如果对方网络断掉，或在机器司机可能收不到EOF或者ERR
	if(what & (BEV_EVENT_EOF|BEV_EVENT_ERROR|BEV_EVENT_TIMEOUT))
	{
		cout << "BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT" << endl;
		//bufferevent_free(bev);
		delete this;
	}
}

//初始化任务，运行在子线程中
bool XFtpServerCMD::Init()
{
	cout << "XFtpServerCMD::Init()" << endl;
	//监听socket bufferevent   base socket
	bufferevent* bev = bufferevent_socket_new(base, sock, BEV_OPT_CLOSE_ON_FREE);
	if(!bev)
	{
		delete this;
		return false;
	}
	this->bev = bev;
	this->SetCallback(bev);
	
	//添加超时 1min
	timeval rt = {60, 0};
	bufferevent_set_timeouts(bev, &rt, 0);

	string msg = "220 Welcome to libevent XFtpServer\r\n";
	bufferevent_write(bev, msg.c_str(), msg.size());
	return true;
}

XFtpServerCMD::XFtpServerCMD()
{

}

XFtpServerCMD::~XFtpServerCMD()
{
	Close();
	for(auto ptr = calls_del.begin(); ptr != calls_del.end(); ptr++)
	{
		ptr->first->Close();
		delete ptr->first;
	}
	// for(auto x : calls_del)
	// {
	// 	x.first->Close();
	// 	delete x.first;
	// }
}
