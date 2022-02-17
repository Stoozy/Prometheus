/* note these headers are all provided by newlib - you don't need to provide them */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>
 
typedef unsigned long long u64;

void _exit(){
    asm volatile("mov $0, %rsi\n\t\
        syscall");
}

int close(int file){
    u64 r8 = (u64)file;
    asm volatile("mov $2, %%rsi\n\t\
            mov %0, %%r8\n\t\
            syscall" :"=r"(r8));

    u64 r15;
    asm volatile("mov %%r15, %0": : "r"(r15));

    return (int)r15;
}

char **environ; /* pointer to array of char * strings that define the current environment variables */
int execve(char *name, char **argv, char **env){
    return -1;
}

int fork(){
    return -1;
}

int fstat(int file, struct stat *st){
    return -1;
}

int getpid(){
    return -1;
}

int isatty(int file){
    return 1;
}

int kill(int pid, int sig){
    return -1;
}

int link(char *old, char *new){
    return -1;
}
int lseek(int file, int ptr, int dir){
    return -1;
}

int open(const char *name, int flags, ...){
    u64 r8 = (u64)name;
    u64 r9 = (u64)flags;
    asm volatile("mov $1, %%rsi\n\t\
            mov %0, %%r8\n\t\
            mov %1, %%r9\n\t\
            syscall" :"=r"(r8),"=r"(r9));

    u64 r15;
    asm volatile("mov %%r15, %0": : "r"(r15));

    return (int)r15;

}

int read(int file, char *ptr, int len){
    u64 r8 = (u64)file;
    u64 r9 = (u64)ptr;
    u64 r10 = (u64) len;
    asm volatile("mov $3, %%rsi\n\t\
            mov %0, %%r8\n\t\
            mov %1, %%r9\n\t\
            mov %2, %%r10\n\t\
            syscall" :"=r"(r8),"=r"(r9),"=r"(r10));

    u64 r15;
    asm volatile("mov %%r15, %0": : "r"(r15));

    return (int)r15;

}

caddr_t sbrk(int incr){
    return NULL;
}

int stat(const char *file, struct stat *st){
    return -1;
}

clock_t times(struct tms *buf){
    return -1;
}

int unlink(char *name){
    return -1;
}

int wait(int *status){
    return -1;
}

int write(int file, char *ptr, int len){
    register int fd_idx asm("r8") = file;
    register char * str asm("r9") = ptr;
    register int length asm("r10") = len;

    asm ("syscall");

    register int ret asm("r15");

    return ret;
}

//int gettimeofday(struct timeval *p, struct timezone *z);
