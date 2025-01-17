#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

/*TIPS1
p = 从左邻居中获取一个数
print p
loop:
    n = 从左邻居中获取一个数
    if (n不能被p整除)
        将n发送给右邻居*/

// xv6 每个进程能打开的文件描述符总数只有 16 个 ,所以要关闭用不到的方向的文件描述符


// void primes(int lp[2])
// {
//     int p0;
//     read(lp[0], &p0, sizeof(int)); // 第一个数据
//     if (p0 == -1){
//         exit(0);
//     }
        

//     printf("prime %d\n", p0);

//     int rp[2]; // 传给右边邻居进程的管道
//     pipe(rp);

//     if (fork() == 0)
//     {
//         close(lp[0]);
//         close(rp[1]);
//         primes(rp);
//     }
//     else
//     {
//         close(rp[0]);
//         int data;
//         while (read(lp[0], &data, sizeof(int)) == sizeof(int))
//         { // 只要管道里还有数据

            
//                 if (data % p0 != 0 || data == -1)/
//                 {
//                     write(rp[1], &data, sizeof(int));
//                 }

//         }

//         close(rp[1]);
//         wait(0);
//     }
// }

void primes(int lp[2])
{
    int p0;
    if(read(lp[0], &p0, sizeof(int)) == 0){
        exit(0);
    }; // 第一个数据,如果管道为空说明是最后一个进程了
    
        

    printf("prime %d\n", p0);

    int rp[2]; // 传给右边邻居进程的管道
    pipe(rp);

    if (fork() == 0)
    {
        close(lp[0]);
        close(rp[1]);
        primes(rp);
    }
    else
    {
        close(rp[0]);
        int data;
        while (read(lp[0], &data, sizeof(int)) == sizeof(int))
        { // 只要管道里还有数据

            if (data % p0 != 0 ) //
            {
                write(rp[1], &data, sizeof(int));
                
            }
        }
        //TIPS2
        //  在子进程中递归调用 primes 时，虽然关闭了当前进程中不需要的 lp[0] 和
        //  rp[1]，但父进程的 rp[1] 仍然打开着。由于管道 rp 的写端未完全关闭，子进程在尝试从
        //  rp[0] 读取时会一直等待，导致死锁或死循环。
        close(rp[1]);
        wait(0);
    }
}

int main(int argc, char **argv)
{
    int p[2]; // [0]读，[1]写
    pipe(p);

    if (fork() == 0)
    {
        close(p[1]);
        primes(p);
        exit(0);
    }
    else
    {

        for (int i = 2; i <= 35; i++)
        {
            write(p[1], &i, sizeof(int));
        }
        // int i = -1;
        // write(p[1], &i, sizeof(int));

        close(p[0]); // 第一个父进程与子进程的管道不会再用到了
        close(p[1]);
    }
    wait(0);
    exit(0);
}