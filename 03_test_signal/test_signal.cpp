/*************************************************************************
 *     > File Name: test_signal.cpp
 *     > Created Time: 2020年12月26日 星期六 21时35分06秒
 * 
 * linux事件状态分析：
 * 已初始化 initialied    调用event_new后
 * 待决     pending      调用event_add后
 * 激活     active       事件发生
 * 持久的   persistent
************************************************************************/
#if 1
#include <iostream>
#include <event2/event.h>
#include <signal.h>

using namespace std;

/* sock 文件描述符
 * which 事件类型
 * arg 传递的参数*/
static void Ctrl_C(int sock, short which, void* arg)
{
	cout << "ctrl + c" << endl;	
}

static void Kill(int sock, short which, void* arg)
{
	cout << "Kill" << endl;

	event* ev = (event*)arg;	
	//如果处于非待决状态，则再次进入
	if(!evsignal_pending(ev, NULL))
	{
		event_del(ev);
		event_add(ev, NULL);
	}
}


int main(int argc, const char** argv)
{
	event_base* base = event_base_new();
	
	//添加信号ctrl+C 信号事件，处于no pending状态
	//evsignal_new隐藏的状态 EV_SIGNAL|EV_PERSIST
	event* csig = evsignal_new(base, SIGINT, Ctrl_C, base);
	if(!csig)
	{
		cerr << "SIGINT evsignale_new failed!" << endl;
		return -1;
	}

	//使事件处于pending状态
	if(event_add(csig, 0) != 0)
	{
		cerr << "SIGINT event_add failed!" << endl;
		return -1;
	}

	//添加kill信号
	//非持久信号，只进入一次, event_self_cbarg()传递当前的event
	event* ksig = event_new(base, SIGTERM, EV_SIGNAL, Kill, event_self_cbarg());
	if(!ksig)
	{
		cerr << "SIGINT evsignale_new failed!" << endl;
		return -1;
	}

	//使事件处于pending状态
	if(event_add(ksig, 0) != 0)
	{
		cerr << "SIGINT event_add failed!" << endl;
		return -1;
	}

	//进入事件主循环
	event_base_dispatch(base);
	event_free(csig);
	event_base_free(base);

	return 0;
}

#endif

#if 0
#include <event.h>
#include <signal.h>

int signal_count = 0;
void signal_handler(evutil_socket_t fd, short events, void *arg)
{
	struct event *ev = (struct event *)arg;
	printf("第 %d 次收到信号 %d\n", signal_count, fd);
	signal_count++;
	if(signal_count >= 3)//收到3次ctrl + c后退出
	{
		//将事件从集合中移除
		event_del(ev);
	}
}
int main()
{
	//创建事件集合
	struct event_base *base = event_base_new();
	//创建事件
	struct event ev;
	//将事件和信号绑定
	event_assign(&ev, base, SIGINT, EV_SIGNAL | EV_PERSIST, signal_handler, &ev);
	//事件添加到集合中
	event_add(&ev, NULL);
	//监听集合
	event_base_dispatch(base);
	//释放集合
	event_base_free(base);
}
#endif