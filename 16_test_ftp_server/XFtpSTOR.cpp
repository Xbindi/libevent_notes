#include "XFtpSTOR.h"
#include <event2/event.h>
#include <event2/bufferevent.h>
using namespace std;


//解析协议
void XFtpSTOR::Parse(std::string type, std::string msg)
{
    //文件名
    int pos = msg.rfind(" ") + 1;
    string filename = msg.substr(pos, msg.size() - pos - 2);
    string path  = cmdTask->rootDir;
    path += cmdTask->curDir;
    path += filename;
    fp = fopen(path.c_str(), "wb");
    if(fp)
    {
        //连接数据通道
        ConnectPORT();
        //发送开始接收文件指令
        ResCMD("150 File OK\r\n");
        //激活读取事件
        bufferevent_trigger(bev, EV_READ, 0);
    }
    else
    {
        ResCMD("450 STOR File open failed\r\n");
    }
}

void XFtpSTOR::Read(struct bufferevent* bev)
{
    if(!fp) return;//文件打开失败
    for(;;)
    {
        int len = bufferevent_read(bev, buf, sizeof(buf));
        if(len <= 0)
            return;
        int size = fwrite(buf, 1, len, fp);
        cout << "<" << len << ":" << size << ">" << flush;
    }
}

void XFtpSTOR::Event(struct bufferevent* bev, short what)
{
    //对方网络断开，或者死机有可能收不到BEV_EVENT_EOF数据
    if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT))
	{
		cout << " what = "<< what << " BEV_EVENT_EOF | BEV_EVENT_ERROR |BEV_EVENT_TIMEOUT" << endl;
		Close();
        ResCMD("226 Transfer complete\r\n");
	}
	else if(what & BEV_EVENT_CONNECTED)
	{
		cout << "XFtpSTOR BEV_EVENT_CONNECTED" << endl;
	}
}



