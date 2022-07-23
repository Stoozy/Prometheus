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

void schedule(Registers *regs) {

  // load_pagedir(kernel_cr3);

  // not enough procs or not time to switch yet
  if (g_procs == 0) {
    return;
  } else if (g_ticks % SMP_TIMESLICE != 0) {
    // load_pagedir(gp_current_process->cr3);
    return;
  }

  gp_current_process->p_stack = (void *)regs;

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
  kprintf(
      "[SCHEDULER]    Switching to proc with stack at %llx and %llx as cr3\n",
      (void *)gp_current_process->p_stack, (void *)gp_current_process->cr3);
#endif

  switch_to_process(gp_current_process->p_stack,
                    (void *)gp_current_process->cr3);
}
