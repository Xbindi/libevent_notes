#include "XThreadPool.h"
#include "XTask.h"
#include "XThread.h"
#include <thread>
#include <iostream>

using namespace std;
//分发线程
void XThreadPool::Dispatch(XTask* task)
{
	if(!task) return ;
	int tid = (lastThread + 1) % threadCount;
	lastThread = tid;
	XThread* t = threads[tid];

	//添加任务
	t->AddTask(task);
	//激活线程
	t->Active();
}


void XThreadPool::Init(int threadCount)
{
	this->threadCount = threadCount;
	lastThread = -1;
	for(int i = 0; i < threadCount; i++)
	{
		XThread* t = new XThread();//新建线程
		cout << "Create thread " << i << endl;
		t->id = i+1;
		t->Start();
		threads.push_back(t);//将线程加入管理
		this_thread::sleep_for(chrono::milliseconds(10));
	}
}
