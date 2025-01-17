// xargs.c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void run(char*program, char** argv){
    if(fork() == 0){
        exec(program,argv);
        exit(0);
    }
}

int main(int argc, char** argv){
    char *linesplit[64];//用于存放所有字符
    for(int j = 1; j < argc ;j++){//argv[0]:xargs
        linesplit[j - 1] = argv[j]; 
    }
    char buf[64];//用于划分字符
    char block[64];
    int l;
    int m = 0,k = argc - 1;//m与buf对应使用，k与linesplict对应使用
    while((l = read(0,block,sizeof(block))) != 0){//0是标准输入
        for(int i = 0; i < l; i++){
            if(block[i] == ' '){
                buf[m] = '\0';
                m = 0;
                linesplit[k++] = buf;
                
            }else if(block[i] == '\n'){
                buf[m] = '\0';
                m = 0;
                linesplit[k++] = buf;
                linesplit[k] = 0;//exec(path， argv)中argv最后一个元素必须是NULL
                k = argc - 1;//清空，准备读取下一行
                if(fork() == 0){
                    exec(argv[1],linesplit);
                }
                
                wait(0);
            }else{
                buf[m++] = block[i];
            }
        }
    }
    exit(0);
}