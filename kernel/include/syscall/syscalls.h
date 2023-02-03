#pragma once

#include <memory/vmm.h>
#include <stddef.h>
#include <libk/typedefs.h>

typedef unsigned long long caddr_t;

/* syscalls */
#define SYS_EXIT 0
#define SYS_OPEN 1
#define SYS_CLOSE 2
#define SYS_READ 3
#define SYS_WRITE 4
#define SYS_LOG_LIBC 5
#define SYS_VM_MAP 6
#define SYS_SEEK 7
#define SYS_TCB_SET 8
#define SYS_IOCTL 9
#define SYS_FORK 10
#define SYS_WAIT 11
#define SYS_STAT 12
#define SYS_FSTAT 13
#define SYS_GETPID 14
#define SYS_DUP 15
#define SYS_DUP2 16
#define SYS_READDIR	17
#define SYS_FCNTL	18
#define SYS_POLL	19
#define SYS_EXEC    20

// GDB stubs
#define SYS_DBG_PUTC    30
#define SYS_DBG_GETC    31
#define SYS_DBG_EXCH    32


void sys_init();
