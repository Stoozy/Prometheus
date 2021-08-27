#ifndef _CPU_H
#define _CPU_H 1

#include "../typedefs.h"

typedef struct  {
	u64 rdi;
    u64 rsi;   
    u64 rbp;   
    u64 rbx;   
    u64 rdx;   
    u64 rcx;
    u64 rax;


    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;

} __attribute__((packed)) Registers;

#endif 
