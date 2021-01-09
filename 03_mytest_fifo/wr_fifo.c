#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

int main()
{
    int ret;
    int fd = open("fifo.tmp", O_WRONLY);
    if(-1 == fd)
    {
        perror("open err\n");
        return -1;
    }
    char buf[32] = {0};
    while(1)
    {
        scanf("%s", buf);
        ret = write(fd, buf, strlen(buf));
        if(-1 == ret)
        {
            perror("write err\n");
            return -1;
        }

        if(!strcmp(buf, "bye"))
        {
            break;
        }

        memset(buf, 0 ,sizeof(buf));
    }
    return 0;
}