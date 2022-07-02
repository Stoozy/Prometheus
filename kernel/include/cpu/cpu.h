#pragma once

#include <typedefs.h>
#include <memory/vmm.h>

typedef struct  {
    u64 rdi;    
    u64 rsi;    
    u64 rax;
    u64 rbx;    
    u64 rcx;    
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
    u64 * syscall_kernel_stack; // 0x0
    u64 * syscall_user_stack;   // 0x8
    PageTable * kcr3;           // 0x10
    PageTable * pcr3;           // 0x18

    Registers regs;
} __attribute__((packed)) LocalCpuData;


void cpu_init(u8);
LocalCpuData * get_cpu_struct(u8);
void dump_regs(Registers * );

