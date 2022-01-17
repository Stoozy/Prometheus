#pragma once

#include "../typedefs.h"

typedef struct  {
	u64 rdi;
    u64 rsi;   
    u64 rbx;   
    u64 rbp;   

    u64 r15;
    u64 r14;
    u64 r13;
    u64 r12;
    u64 r11;
    u64 r10;
    u64 r9;
    u64 r8;

    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
} __attribute__((packed)) Registers;

typedef struct {
    u64 * syscall_stack;
    u64 saved_proc_stack;
    // space for other things in the future
} __attribute__((packed)) CpuData;


void cpu_init(u8);
CpuData * get_cpu_struct(u8);
void dump_regs(Registers * );

