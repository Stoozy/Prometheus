#include "cpu/cpu.h"
#include "memory/vmm.h"
#include <config.h>
#include <cpu/idt.h>
#include <libk/kmalloc.h>
#include <libk/kprintf.h>
#include <proc/proc.h>

extern void switch_to_process(void *new_stack, PageTable *cr3);
extern void load_pagedir();

extern u64 g_ticks;

extern PageTable *kernel_cr3;

extern ProcessQueue ready_queue;
extern ProcessQueue wait_queue;
extern volatile ProcessControlBlock *running;

ProcessControlBlock *get_next_ready_process() {
    extern void dump_pqueue(ProcessQueue *);
    for(PQNode * cnode = ready_queue.first; cnode; cnode = cnode->next){
        if(cnode->pcb->pid == running->pid){
            return cnode->next == NULL ? ready_queue.first->pcb : cnode->next->pcb;
        }
    }

    return ready_queue.first->pcb;
}

void schedule(Registers *regs) {
    asm("cli");
    // not enough procs
    if (ready_queue.count == 0)
        return;

    // save registers
    running->trapframe = *regs;
    running->fs_base = rdmsr(FSBASE);

    running = get_next_ready_process();
    running->state = RUNNING;

#ifdef SCHEDULER_DEBUG
    extern void dump_pqueue(ProcessQueue *);

    // kprintf("Ready Queue\n");
    // dump_pqueue(&ready_queue);

    kprintf("Switching to %s (%d); cr3 0x%x\n", running->name, running->pid,
            running->cr3);
    // kprintf("Trapframe at  %x\n", &running->trapframe);
    // dump_regs(&running->trapframe);

#endif



    wrmsr(FSBASE, running->fs_base);
    get_cpu_struct(0)->syscall_kernel_stack = running->kstack + PAGING_VIRTUAL_OFFSET;
    switch_to_process(&running->trapframe, (void *)running->cr3);
}
