#pragma once

#include <stddef.h>
#include "../typedefs.h"
#include "../memory/vmm.h"

typedef unsigned long long caddr_t;

#define EFER        0xC0000080

#define STAR        0xC0000081
#define LSTAR       0xC0000082
#define CSTAR       0xC0000083
#define SFMASK      0xC0000084

#define FSBASE      0xC0000100
#define GSBASE      0xC0000101
#define KGSBASE     0xC0000102


/* syscalls */
#define SYS_EXIT        0
#define SYS_OPEN        1
#define SYS_CLOSE       2
#define SYS_READ        3
#define SYS_WRITE       4
#define SYS_LOG_LIBC    5
#define SYS_MMAP        6

int sys_exit();
int sys_close(int file);
//char ** environ; /* pointer to array of char * strings that define the current environment variables */
int sys_execve(char *name, char **argv, char **env);
int sys_fork();
int sys_fstat(int file, struct stat *st);
int sys_getpid();
int sys_isatty(int file);
int sys_kill(int pid, int sig);
int sys_link(char *old, char * _new);
int sys_lseek(int file, int ptr, int dir);
int sys_open(const char *name, int flags, ...);
int sys_read(int file, char *ptr, size_t len);

caddr_t sys_sbrk(int incr);
int sys_stat(const char *file, struct stat *st);
//clock_t times(struct tms *buf);
int sys_unlink(char *name);
int sys_wait(int *status);
int sys_write(int file, char *ptr, int len);
int sys_gettimeofday(struct timeval *p, struct timezone *z);

void syscall_dispatcher();
void sys_init();
