#include <cpu/cpu.h>
#include <libk/kprintf.h>
#include <libk/typedefs.h>
#include <memory/pmm.h>
#include <memory/vmm.h>

#define MAX_CORES 255

LocalCpuData cpus[MAX_CORES];

void cpu_init(u8 id) {
    kprintf("Initializing CPU #%lu\n", id);

    void *addr = pmm_alloc_blocks(16) + (16 * PAGE_SIZE);

    extern void panic();
    if (addr != NULL) {
        cpus[id].syscall_kernel_stack = (u64 *)(PAGING_VIRTUAL_OFFSET + addr);
        kprintf("kernel stack at 0x%x\n", addr);
        kprintf("kernel stack base at 0x%x\n", addr - (16 * PAGE_SIZE));
    } else {
        kprintf("OUT OF MEMORY\n");
        panic();
    }
    cpus[id].kcr3 = vmm_get_current_cr3();

    return;
}

LocalCpuData *get_cpu_struct(u8 id) { return &cpus[id]; }

void dump_regs(Registers *regs) {
    kprintf("REGISTERS: \n");
    kprintf("     RIP: 0x%llx\n", regs->rip);
    kprintf("     RSP: 0x%llx\n", regs->rsp);
    kprintf("     RBP: 0x%llx\n", regs->rbp);
    kprintf("     RAX: 0x%llx\n", regs->rax);
    kprintf("     RBX: 0x%llx\n", regs->rbx);
    kprintf("     RCX: 0x%llx\n", regs->rcx);
    kprintf("     RDX: 0x%llx\n", regs->rdx);
    kprintf("     RSI: 0x%llx\n", regs->rsi);
    kprintf("     RDI: 0x%llx\n", regs->rdi);

    kprintf("     R8: 0x%llx\n", regs->r8);
    kprintf("     R9: 0x%llx\n", regs->r9);
    kprintf("     R10: 0x%llx\n", regs->r10);
    kprintf("     R11: 0x%llx\n", regs->r11);
    kprintf("     R12: 0x%llx\n", regs->r12);
    kprintf("     R13: 0x%llx\n", regs->r13);
    kprintf("     R14: 0x%llx\n", regs->r14);
    kprintf("     R15: 0x%llx\n", regs->r15);

    kprintf("     RFLAGS: 0x%llx\n", regs->rflags);
    kprintf("     SS: 0x%llx\n", regs->ss);
    kprintf("     CS: 0x%llx\n", regs->cs);
}
