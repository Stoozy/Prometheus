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
    // if (running->state == WAITING) {
    //   ProcessControlBlock *next =
    //       running->next == NULL ? ready_queue.first : running->next;

    //  kprintf("Removing %x from ready queue\n", running);
    //  pqueue_remove(&ready_queue, running);
    //  dump_pqueue(&ready_queue);
    //  return next;
    //}

    return running->next == NULL ? ready_queue.first : running->next;
}

void schedule(Registers *regs) {
    asm("cli");
    // not enough procs
    if (ready_queue.count == 0)
        return;

    // save registers
    running->trapframe = *regs;

    running = get_next_ready_process();
    running->state = RUNNING;

#ifdef SCHEDULER_DEBUG
    extern void dump_pqueue(ProcessQueue *);

    // kprintf("Ready Queue\n");
    // dump_pqueue(&ready_queue);

    kprintf("Switching to %s (%d); cr3 0x%x\n", running->name, running->pid,
            running->cr3);
    // kprintf("Trapframe at  %x\n", &running->trapframe);
    dump_regs(&running->trapframe);
#endif

    switch_to_process(&running->trapframe, (void *)running->cr3);
}
