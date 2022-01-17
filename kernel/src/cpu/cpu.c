#include "cpu.h"
#include "../typedefs.h"
#include "../kprintf.h"
#include "../memory/pmm.h"

#define MAX_CORES   255

CpuData cpus[MAX_CORES];

void cpu_init(u8 id){
    cpus[id].syscall_stack = (u64*)pmm_alloc_block() + 0x1000;
}

CpuData * get_cpu_struct(u8 id){
    return &cpus[id];
}

void dump_regs(Registers * regs){
    kprintf("[SCHEDULER]    RIP: 0x%llx\n", regs->rip);
    kprintf("[SCHEDULER]    RSP: 0x%llx\n", regs->rsp);
    kprintf("[SCHEDULER]    RBP: 0x%llx\n", regs->rbp);
    kprintf("[SCHEDULER]    RBX: 0x%llx\n", regs->rbx);
    kprintf("[SCHEDULER]    RSI: 0x%llx\n", regs->rsi);
    kprintf("[SCHEDULER]    RDI: 0x%llx\n", regs->rdi);

    kprintf("[SCHEDULER]    R8: 0x%llx\n", regs->r8);
    kprintf("[SCHEDULER]    R9: 0x%llx\n", regs->r9);
    kprintf("[SCHEDULER]    R10: 0x%llx\n", regs->r10);
    kprintf("[SCHEDULER]    R11: 0x%llx\n", regs->r11);
    kprintf("[SCHEDULER]    R12: 0x%llx\n", regs->r12);
    kprintf("[SCHEDULER]    R13: 0x%llx\n", regs->r13);
    kprintf("[SCHEDULER]    R14: 0x%llx\n", regs->r14);
    kprintf("[SCHEDULER]    R15: 0x%llx\n", regs->r15);

    kprintf("[SCHEDULER]    RFLAGS: 0x%llx\n", regs->rflags);
    kprintf("[SCHEDULER]    SS: 0x%llx\n", regs->ss);
    kprintf("[SCHEDULER]    CS: 0x%llx\n", regs->cs);
}
