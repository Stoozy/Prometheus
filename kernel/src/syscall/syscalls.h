#pragma once

#include "../memory/vmm.h"
#include "../typedefs.h"
#include <stddef.h>

typedef unsigned long long caddr_t;

#define EFER 0xC0000080

#define STAR 0xC0000081
#define LSTAR 0xC0000082
#define CSTAR 0xC0000083
#define SFMASK 0xC0000084

#define FSBASE 0xC0000100
#define GSBASE 0xC0000101
#define KGSBASE 0xC0000102

/* syscalls */
#define SYS_EXIT 0
#define SYS_OPEN 1
#define SYS_CLOSE 2
#define SYS_READ 3
#define SYS_WRITE 4
#define SYS_LOG_LIBC 5
#define SYS_VM_MAP 6
#define SYS_SEEK 7

void sys_init();
