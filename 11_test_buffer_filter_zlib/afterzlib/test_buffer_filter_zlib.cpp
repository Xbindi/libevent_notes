/**********************************************************
 * 客户端读取文件压缩处理后（在输出过滤中处理）发送数据，
 * 服务端接收数据解压处理（在输入过滤中处理），并统计数据
 * 
 * *******************************************************/

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <iostream>
#include <signal.h>
#include <string.h>

using namespace std;

void Server(event_base* base);
void Client(event_base* base);

int main()
{
	//忽略管道信号，发送数据给以关闭的
	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
	{
		return 1;
	}

	event_base* base = event_base_new();
	if(base)
	{
		cout << "event_base_new success!" << endl;
	}

	cout << "test server" << endl;
	Server(base);

	cout << "test client" << endl;
	Client(base);
	
	event_base_dispatch(base);

	cout << "end!" << endl;
	return 0;
}
