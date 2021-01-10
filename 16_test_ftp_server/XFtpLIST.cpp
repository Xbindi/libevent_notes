#include "XFtpLIST.h"
#include <iostream>
#include <string>
#include <event2/bufferevent.h>
#include <event2/event.h>
using namespace std;

void XFtpLIST::Write(struct bufferevent* bev)
{
	cout << "XFtpLIST:: Write" << endl;
	//4 发送完成226 
	ResCMD("226 Transfer complete.\r\n");
	//5 关闭链接
	Close();
}



void XFtpLIST::Event(struct bufferevent *bev, short what)
{
	if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT))
	{
		cout << " what = "<< what << " BEV_EVENT_EOF | BEV_EVENT_ERROR |BEV_EVENT_TIMEOUT" << endl;
		Close();
	}
	else if(what & BEV_EVENT_CONNECTED)
	{
		cout << "XFtpLIST BEV_EVENT_CONNECTED" << endl;
	}
}

	//解析协议
void XFtpLIST::Parse(std::string type, std::string msg)
{
	cout << "XFtpLIST:: Parse type = " << type << " msg = " << msg << endl;
	string remsg = "";
	if(type == "PWD")
	{
		remsg = "257 \"";
		remsg += cmdTask->curDir;
		remsg += "\" is current dir.\r\n";
		ResCMD(remsg);	//回复消息
	}
	else if(type == "LIST")
	{
		//1链接数据通道 2 150回应 3 发送目录数据通道 4 发送完成226 5 关闭链接

		//1链接数据通道
		ConnectPORT();
		//2 150回应
		ResCMD("150 Here comes the directory listing.\r\n");
		//string listdata = "-rwxrwxrwx 1 root group 64463 Mar 14 09:53 101.jpg.\r\n";
		//回复消息， 使用数据通道发送目录
		string listdata = GetListData(cmdTask->rootDir + cmdTask->curDir);
		//3 链接数据通道
		Send(listdata);
		//4、5进入XFtpLIST::Write完成
	}

	else if(type == "CWD")  //切换目录
	{
		//取出命令中的路径 CWD test\r\n
		int pos = msg.rfind(" ") + 1;
		//去掉结尾的\r\n
		string path = msg.substr(pos, msg.size() - pos - 2);
		if(path[0] == '/')//绝对路径
		{
			cmdTask->curDir = path;
		}
		else
		{
			if(cmdTask->curDir[cmdTask->curDir.size() - 1] != '/')
				cmdTask ->curDir += "/";
			cmdTask->curDir += path + "/";
		}	
		//消息回复
		ResCMD("250 Directory success changed!.\r\n");

	}
	else if(type == "CDUP")  //返回上层目录
	{
		// /dir/test.txt /dir/ /dir
		string path = cmdTask->curDir;
		//统一去掉结尾的 /
		if(path[path.size() - 1] == '/')
		{
			path = path.substr(0, path.size() - 1);
		}
		int pos = path.rfind("/");
		path = path.substr(0, pos);
		cmdTask ->curDir = path;
		//消息回复
		ResCMD("250 Directory success changed!.\r\n");
	}

}

string XFtpLIST::GetListData(string path)
{
	string data = "";
	//-rwxrwxrwx 1 root group 64463 Mar 14 09:53 101.jpg.\r\n
	string cmd = "ls -l ";
	cmd += path;
	cout << "popen:" << cmd << endl;
	FILE *f = popen(cmd.c_str(), "r");
	if(!f)
		return data;
	char buffer[1024] = {0};
	for(;;)
	{
		int len = fread(buffer, 1, sizeof(buffer)-1, f);
		if(len <= 0) break;
		buffer[len] = '\0';
		data += buffer;
	}
	pclose(f);

	return data;
}