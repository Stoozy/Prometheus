#include "memory/vmm.h"
#include <config.h>
#include <kmalloc.h>
#include <kprintf.h>
#include <proc/proc.h>

extern void switch_to_process(void *new_stack, PageTable *cr3);
extern void load_pagedir();

extern u64 g_ticks;

extern PageTable *kernel_cr3;

extern ProcessQueue ready_queue;
extern ProcessQueue wait_queue;
extern ProcessControlBlock *running;

void schedule(Registers *regs) {
  // not enough procs
  if (ready_queue.count == 0)
    return;

  // save registers
  running->trapframe = *regs;

  extern void dump_queue(ProcessQueue *);
  if (running->state == WAITING) {
    ProcessControlBlock *next =
        running->next == NULL ? ready_queue.first : running->next;

    kprintf("Removing %x from ready queue\n", running);
    pqueue_remove(&ready_queue, running);
    dump_queue(&ready_queue);
    running = next;
    goto next;
  }

  running = running->next == NULL ? ready_queue.first : running->next;

next:

  running->state = RUNNING;

#ifdef SCHEDULER_DEBUG
  extern void dump_queue(ProcessQueue *);

  kprintf("Ready Queue\n");
  dump_queue(&ready_queue);

  kprintf("[SCHEDULER]    Switching to proc with trapframe at %llx and %llx as "
          "cr3\n",
          running->trapframe, (void *)running->cr3);
#endif

  // finally, switch
  switch_to_process(&running->trapframe, (void *)running->cr3);
}
