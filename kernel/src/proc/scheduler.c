#include "cpu/cpu.h"
#include "kmalloc.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include <config.h>
#include <kprintf.h>
#include <proc/proc.h>
#include <proc/scheduler.h>

extern void switch_to_process(void *new_stack, PageTable *cr3);
extern uint64_t g_ticks;

ProcessControlBlock *gp_current_process;

ProcessQueue process_queue;

void dump_process_queue() {
  kprintf("PROCESS QUEUE\n");
  kprintf("There are %d processes in queue\n", process_queue.count);
  for (PCBNode *cnode = process_queue.front; cnode; cnode = cnode->next) {

    kprintf("Process at %x\n", &cnode->pcb);
    dump_regs(&cnode->pcb->trapframe);
  }
}

static bool is_process_queue_empty() { return !process_queue.count; }

void enqueue_process(ProcessControlBlock *pcb) {
  PCBNode *new_node = pmm_alloc_block();
  // kmalloc(sizeof(PCBNode));
  new_node->pcb = pcb;
  new_node->next = NULL;

  if (process_queue.rear == NULL)
    process_queue.front = process_queue.rear = new_node;

  process_queue.rear->next = new_node;
  process_queue.rear = process_queue.rear->next;
  process_queue.count++;
}

void dequeue_process() {
  if (process_queue.front == NULL)
    return;

  PCBNode *tmp = process_queue.front;
  pmm_free_block((uintptr_t)tmp);

  process_queue.front = tmp->next;

  if (process_queue.front == NULL)
    process_queue.rear = NULL;

  // kfree(tmp);
  process_queue.count--;
}

void block_process(ProcessControlBlock *pcb) { pcb->state = WAITING; }
void unblock_process(ProcessControlBlock *pcb) { pcb->state = READY; }

void schedule(Registers *regs) {
  if (g_ticks % PROC_TIMESLICE != 0)
    return;

  dump_process_queue();
  while (is_process_queue_empty())
    kprintf("Nothing to run ...\n");

  PCBNode *next = process_queue.front;
  next->pcb->trapframe = *regs;
  enqueue_process(next->pcb);
  dequeue_process();

#ifdef SCHEDULER_DEBUG
  kprintf("[SCHEDULER]  Switching to next process\n");
  dump_regs(&next->pcb->trapframe);
#endif

  gp_current_process = next->pcb;

  switch_to_process(&next->pcb->trapframe, next->pcb->cr3);
}

void scheduler_init() {
  process_queue.count = 0;
  process_queue.front = NULL;
  process_queue.rear = NULL;
}
