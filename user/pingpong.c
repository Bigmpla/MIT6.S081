#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char **argv)
{
    //pp2c用于父进程向子进程发送数据的管道
    //pc2p用于子进程向父进程发送数据的管道
    int pp2c[2], pc2p[2]; // [0]读，[1]写
    pipe(pp2c);
    pipe(pc2p);

    if (fork() == 0)
    {
        char c;
        read(pp2c[0], &c, 1);
        printf("%d: received ping\n", getpid());
        write(pc2p[1], &c, 1);
    }
    else
    {
        char c;
        write(pp2c[1], "a", 1);
        read(pc2p[0], &c, 1);
        printf("%d: received pong\n", getpid());
    }
    exit(0);
}