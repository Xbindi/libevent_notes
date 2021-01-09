/*************************************************************
 * 程序说明：
 * 1. 结合管道fifo测试libevent使用
 * 2. 先执行rd创建管道文件，并使用libevent的事件对象监听该管道
 * 3. 再新开终端执行wr对管道进行写入数据，即可发现rd程序有对应数据输出
 * 4. make编译程序，make clean清除临时管道文件以及可执行文件
 * **********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <event.h>

//当监听事件满足条件时会触发回调函数，通过回调函数读取数据
void fifo_read(evutil_socket_t fd, short events, void *arg)
{
    char buf[32] = {0};
    int ret = read(fd, buf, sizeof(buf));
    if(-1 == ret)
    {
        perror("read err\n");
        return;
    }

    printf("read from fifo: %s\n", buf);
}

int main()
{
    if(access("fifo.tmp", F_OK)  == 0)
    {
        remove("fifo.tmp");
    }
    int ret = mkfifo("fifo.tmp", 0700);
    if(-1 == ret)
    {
        perror("mkfifo err\n");
        return -1;
    }

    int fd = open("fifo.tmp", O_RDONLY);
    if(-1 == fd)
    {
        perror("open fifo err\n");
        return -1;
    }

    //创建事件
    struct event ev;
    //创建事件集合
    event_init();
    //初始化事件 绑定fd和ev 参数：事件 关联的文件描述符 事件类型 回调函数 回调函数参数
    event_set(&ev, fd, EV_READ | EV_PERSIST, fifo_read, NULL);
    //将事件添加到集合中
    event_add(&ev, NULL);
    //开始监听  死循环  如果集合中没有事件可以监听，则返回
    event_dispatch();
    return 0;
}