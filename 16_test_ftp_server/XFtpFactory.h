#pragma once

#include "XTask.h"

class XFtpFactory
{
public:
	//单例模式 创建对象
	static XFtpFactory* Get()
	{
		static XFtpFactory f;
		return &f;
	}
	
	XTask* CreateTask();
private:
	XFtpFactory();
};
