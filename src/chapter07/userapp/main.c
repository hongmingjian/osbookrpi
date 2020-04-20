/*
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>

#include "syscall.h"

char stkA[4096];
void tsk_A(void *pv)
{
    int n = (int)pv;
    while(n) {
        printf("A");
        n--;
    }
    task_exit(0);
}

char stkB[4096];
void tsk_B(void *pv)
{
    int n = (int)pv;
    while(n) {
        printf("B");
        n--;
    }
    task_exit(0);
}

char stkH[4096];
void heartbeat(void *pv)
{
    int fd=open("$:/led0", 0);
    int val=1;
    if(fd >= 0) {
        while(1) {
            ioctl(fd, val, NULL);
            val ^= 1;
            sleep(1);
        }
        close(fd);
    } else
        printf("Failed to open $:/led0\r\n");

    task_exit(0);
}

/**
 * 第一个运行在用户模式的线程所执行的函数
 */
int main(void *pv)
{
    open("$:/uart0", O_RDONLY);//0=STDIN_FILENO
    open("$:/uart0", O_WRONLY);//1=STDOUT_FILENO
    open("$:/uart0", O_WRONLY);//2=STDERR_FILENO

    printf("task #%d: I'm the first user task(pv=0x%08x)!\r\n",
            task_getid(), pv);

    int H = task_create(&stkH[4096], &heartbeat, (void *)0);
    int A = task_create(&stkA[4096], tsk_A, (void *)1000);
    int B = task_create(&stkB[4096], tsk_B, (void *)1000);

    task_wait(A, NULL);
    printf("A exited\r\n");
    task_wait(B, NULL);
    printf("B exited\r\n");

    while(1) {
        char c;
        if(1 == read(STDIN_FILENO, &c, 1)) {
            write(STDOUT_FILENO, &c, 1);
        }
    }

    return 0;
}
