#include <config.h>
#include <kprintf.h>
#include <proc/proc.h>

extern void switch_to_process(void *new_stack, PageTable *cr3);
extern void load_pagedir();
extern u64 g_ticks;
extern u64 g_procs;
extern ProcessControlBlock *gp_current_process;
extern ProcessControlBlock *gp_process_queue;

extern PageTable *kernel_cr3;

void save_context(volatile ProcessControlBlock *proc, Registers *regs) {

  proc->p_stack = (void *)regs->rsp;

  u64 *stack = proc->p_stack;
  *--stack = regs->ss;
  *--stack = (u64)proc->p_stack;
  *--stack = regs->rflags;
  *--stack = regs->cs;

  *--stack = regs->rip;
  *--stack = regs->r8;
  *--stack = regs->r9;
  *--stack = regs->r10;
  *--stack = regs->r11;
  *--stack = regs->r12;
  *--stack = regs->r13;
  *--stack = regs->r14;
  *--stack = regs->r15;

  *--stack = regs->rbp;

  *--stack = regs->rdx;
  *--stack = regs->rcx;
  *--stack = regs->rbx;
  *--stack = regs->rax;
  *--stack = regs->rsi;
  *--stack = regs->rdi;

  proc->p_stack = stack;

  // TODO: this should use actual processor ids
  // update LocalCpuData
  LocalCpuData *lcd = get_cpu_struct(0);

  lcd->regs = *regs;
  lcd->syscall_user_stack = stack;

  return;
}

void schedule(Registers *regs) {

#ifdef SCHEDULER_DEBUG
  dump_regs(regs);
  kprintf("[SCHEDULER]    %d Global Processes\n", g_procs);
#endif

  load_pagedir(kernel_cr3); // need it for
  save_context(gp_current_process, regs);

  // not enough procs or not time to switch yet
  if (g_procs == 0) {
    return;
  } else if (g_ticks % SMP_TIMESLICE != 0) {
    load_pagedir(gp_current_process->cr3);
    return;
  }

  if (gp_current_process->next == NULL) {
    gp_current_process = gp_process_queue; // go to head

#ifdef SCHEDULER_DEBUG
    kprintf("[SCHEDULER] Switching to head:\n");
    dump_regs((Registers *)gp_current_process->p_stack);
#endif

  } else if (gp_current_process->next != NULL) {
    // move to next proc
    gp_current_process = gp_current_process->next;
#ifdef SCHEDULER_DEBUG
    kprintf("[SCHEDULER] Switching to next: \n");
    dump_regs(gp_current_process->p_stack);
#endif
  }

#ifdef SCHEDULER_DEBUG
  kprintf("[SCHEDULER]    Switching to proc with %llx as cr3\n",
          (void *)gp_current_process->cr3);
#endif

  switch_to_process(gp_current_process->p_stack,
                    (void *)gp_current_process->cr3);
}
