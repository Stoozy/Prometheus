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
    asm volatile("mov $0, %rdi\n\t\ 
        syscall");
}

int close(int file){
    u64 r8 = (u64)file;
    asm volatile("mov $2, %%rdi\n\t\ 
            mov %0, %%r8\n\t\
            syscall" :"=r"(r8));

    u64 r15;
    asm volatile("mov %%r15, %0": : "r"(r15));

    return (int)r15;
}

char **environ; /* pointer to array of char * strings that define the current environment variables */
int execve(char *name, char **argv, char **env);
int fork();
int fstat(int file, struct stat *st);
int getpid();
int isatty(int file);
int kill(int pid, int sig);
int link(char *old, char *new);
int lseek(int file, int ptr, int dir);

int open(const char *name, int flags, ...){
    u64 r8 = (u64)name;
    u64 r9 = (u64)flags;
    asm volatile("mov $2, %%rdi\n\t\ 
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
    asm volatile("mov $2, %%rdi\n\t\ 
            mov %0, %%r8\n\t\
            mov %1, %%r9\n\t\
            mov %2, %%r10\n\t\
            syscall" :"=r"(r8),"=r"(r9),"=r"(r10));

    u64 r15;
    asm volatile("mov %%r15, %0": : "r"(r15));

    return (int)r15;
}

caddr_t sbrk(int incr);
int stat(const char *file, struct stat *st);
clock_t times(struct tms *buf);
int unlink(char *name);
int wait(int *status);
int write(int file, char *ptr, int len);
//int gettimeofday(struct timeval *p, struct timezone *z);
