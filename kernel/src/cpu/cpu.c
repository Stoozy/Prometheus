#include "cpu.h"
#include "../typedefs.h"
#include "../kprintf.h"
#include "../memory/pmm.h"

#define MAX_CORES   255

LocalCpuData cpus[MAX_CORES];

void cpu_init(u8 id){
    kprintf("Initializing CPU #%lu\n", id);

    void * addr = pmm_alloc_blocks(4) + 4 * PAGE_SIZE;

    extern void panic();
    if(addr != NULL){
        cpus[id].syscall_kernel_stack = (u64*)addr; 
        kprintf("kernel stack at 0x%x\n", addr);
    }else {
        kprintf("OUT OF MEMORY\n");
        panic();
    }
    cpus[id].kcr3  = vmm_get_current_cr3();

    return;
}

LocalCpuData * get_cpu_struct(u8 id){
    return &cpus[id];
}

void dump_regs(Registers * regs){
    kprintf("[REGISTERS]    RIP: 0x%llx\n", regs->rip);
    kprintf("[REGISTERS]    RSP: 0x%llx\n", regs->rsp);
    kprintf("[REGISTERS]    RBP: 0x%llx\n", regs->rbp);
    kprintf("[REGISTERS]    RBX: 0x%llx\n", regs->rbx);
    kprintf("[REGISTERS]    RSI: 0x%llx\n", regs->rsi);
    kprintf("[REGISTERS]    RDI: 0x%llx\n", regs->rdi);

    kprintf("[REGISTERS]    R8: 0x%llx\n", regs->r8);
    kprintf("[REGISTERS]    R9: 0x%llx\n", regs->r9);
    kprintf("[REGISTERS]    R10: 0x%llx\n", regs->r10);
    kprintf("[REGISTERS]    R11: 0x%llx\n", regs->r11);
    kprintf("[REGISTERS]    R12: 0x%llx\n", regs->r12);
    kprintf("[REGISTERS]    R13: 0x%llx\n", regs->r13);
    kprintf("[REGISTERS]    R14: 0x%llx\n", regs->r14);
    kprintf("[REGISTERS]    R15: 0x%llx\n", regs->r15);

    kprintf("[REGISTERS]    RFLAGS: 0x%llx\n", regs->rflags);
    kprintf("[REGISTERS]    SS: 0x%llx\n", regs->ss);
    kprintf("[REGISTERS]    CS: 0x%llx\n", regs->cs);
}
