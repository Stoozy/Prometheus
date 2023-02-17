#include "cpu/cpu.h"
#include "cpu/idt.h"
#include "libk/util.h"
#include "memory/vmm.h"
#include <config.h>
#include <drivers/video.h>
#include <fs/vfs.h>
#include <libk/kmalloc.h>
#include <libk/kprintf.h>
#include <memory/pmm.h>
#include <proc/elf.h>
#include <proc/proc.h>
#include <stdint.h>
#include <string/string.h>

extern u64 g_ticks;
extern void switch_to_process(Registers *new_stack, PageTable *cr3);
extern void load_pagedir();

ProcessQueue ready_queue = {0, NULL, NULL};
ProcessQueue wait_queue = {0, NULL, NULL};
ProcessControlBlock *running = NULL;

uint64_t pid_counter = 200;

void dump_pqueue(ProcessQueue *q) {
  PQNode *cnode = q->first;
  for (; cnode; cnode = cnode->next) {
    kprintf("%s [%d] -> ", cnode->pcb->name, cnode->pcb->pid);
  }
  kprintf("NULL \n");
}

void pqueue_push(ProcessQueue *queue, ProcessControlBlock *proc) {
  PQNode *new_node = kmalloc(sizeof(PQNode));
  new_node->pcb = proc;
  new_node->next = NULL;

  if (!queue->first) {
    queue->count = 1;
    queue->first = queue->last = new_node;
    return;
  }

  queue->last->next = new_node;
  queue->last = queue->last->next;
  queue->count++;
  return;
}

PQNode *pqueue_pop(ProcessQueue *queue) {
  if (!queue->first)
    return NULL;

  // clean this up
  PQNode *tmp = queue->first;

  queue->first = queue->first->next;
  queue->count--;

  return tmp;
}

void pqueue_remove(ProcessQueue *queue, int pid) {
  PQNode *v = pqueue_pop(queue);
  while (v && v->pcb->pid != pid) {
    pqueue_push(queue, v->pcb);
    v = pqueue_pop(queue);
  }
}

void unmap_fd_from_proc(ProcessControlBlock *proc, int fd) {
  if (fd > MAX_PROC_FDS || fd < 0)
    return;

  proc->fd_table[fd] = NULL;
  --proc->fd_length;
  return;
}

int map_file_to_proc(ProcessControlBlock *proc, struct file *file) {
  for (uint64_t fd = 3; fd < MAX_PROC_FDS; ++fd) {
    if (proc->fd_table[fd] == NULL) {
      proc->fd_table[fd] = file;
      ++proc->fd_length;
      return fd;
    }
  }

  return -1;
}

void kill_proc(ProcessControlBlock *proc, int exit_code) {
  // TODO: cleanup actual process

  disable_irq();
  kprintf("Before removing\n");
  dump_pqueue(&ready_queue);
  pqueue_remove(&ready_queue, proc->pid);

  proc->exit_code = exit_code;
  proc->state = ZOMBIE;

  proc->exit_code = exit_code;

  if (proc->parent) {
    proc->parent->childDied = true;
  }

  kprintf("After removing\n");
  dump_pqueue(&ready_queue);

  enable_irq();
  for (;;)
    ;
}

void kill_cur_proc(int exit_code) { kill_proc(running, exit_code); }

void task_a() {
  extern uint8_t kbd_read_from_buffer();
  for (;;) {
    kprintf("Running task A ...\n");
  }
}

void task_b() {
  for (;;)
    kprintf("Running task B...\n");
}
void idle_task() {
  for (;;)
    kprintf("Idling...\n");
}

void dump_proc_vas(ProcessControlBlock *proc) {
  VASRangeNode *cnode = proc->vas;
  kprintf("---------PROCESS VAS---------\n");

  if (proc->vas) {
    kprintf("start: %x; size: %x; flags: %d\n", cnode->virt_start, cnode->size,
            cnode->page_flags);

    while (cnode->next != NULL) {
      kprintf("start: %x; size: %x; flags: %d\n", cnode->virt_start,
              cnode->size, cnode->page_flags);

      cnode = cnode->next;
    }
  }
}

void proc_add_vas_range(ProcessControlBlock *proc, VASRangeNode *node) {
  if (!proc->vas)
    proc->vas = node;

  VASRangeNode *cnode = proc->vas;

  while (cnode->next != NULL)
    cnode = cnode->next;

  cnode->next = node;
  node->next = NULL;
}

ProcessControlBlock *create_kernel_process(void (*entry)(void), char *name) {

  ProcessControlBlock *pcb = kmalloc(sizeof(ProcessControlBlock));
  memset(pcb, 0, sizeof(ProcessControlBlock));

  memcpy(&pcb->name, name, 256);

  void *stack_ptr = pmm_alloc_blocks(STACK_BLOCKS) + STACK_SIZE;
  pcb->kstack = stack_ptr;
  memset(&pcb->trapframe, 0, sizeof(Registers));

  pcb->trapframe.ss = 0x10;
  pcb->trapframe.rsp = (uint64_t)stack_ptr;
  pcb->trapframe.rflags = 0x202;
  pcb->trapframe.cs = 0x08;
  pcb->trapframe.rip = (uint64_t)entry;

  pcb->cr3 = vmm_get_current_cr3(); // kernel cr3
  pcb->state = READY;
  pcb->pid = pid_counter++;

  return pcb;
}

ProcessControlBlock *clone_process(ProcessControlBlock *proc, Registers *regs) {

  ProcessControlBlock *clone = kmalloc(sizeof(ProcessControlBlock));
  *clone = *proc;

  // reset vas so that proper phys addrs get put by vmm_copy_vas
  clone->vas = NULL;

  vmm_copy_vas(clone, proc);

  clone->pid = pid_counter++;

  clone->trapframe = *regs;
  clone->trapframe.rax = 0;

  kprintf("Registers are at stack %x\n", regs);
  kprintf("Fork'd process has cr3: %x\n", clone->cr3);

  pqueue_push(&proc->children, clone);

  clone->parent = proc;

  return clone;
}

void register_process(ProcessControlBlock *new_pcb) {
  pqueue_push(&ready_queue, new_pcb);
  return;
}

void block_process(ProcessControlBlock *proc, int reason) {
  // scheduler will handle the rest
  // note: the rest isn't done here
  // because the registers have to
  // be saved once the interrupt fires
  proc->state = reason;
  kprintf("Blocking process at %x\n", proc);

  pqueue_remove(&ready_queue, proc->pid);

  asm("sti; int $41; cli");
}

void unblock_process(ProcessControlBlock *proc) {
  proc->state = READY;
  pqueue_push(&ready_queue, proc);
  kprintf("\n unblocked %s (%d)\n\n", proc->name, proc->pid);
  return;
}

void multitasking_init() {
  // save kernel page tables

  memset(&ready_queue, 0, sizeof(ProcessQueue));
  memset(&wait_queue, 0, sizeof(ProcessQueue));

  // ProcessControlBlock *nomterm =
  //     create_elf_process("/usr/bin/nomterm", argvp, envp);
  // kprintf("Got process at %x\n", nomterm);
  // register_process(nomterm);

  // extern void terminal_main();

  // register_process(create_kernel_process(task_a, "Task A"));
  // register_process(create_kernel_process(idle_task, "Idle"));
  // register_process(create_kernel_process(task_b, "Task B"));

  extern void fb_proc();

  char *argv[2] = {"/usr/bin/nomterm", NULL};
  char *envp[2] = {"PATH=/usr/bin", NULL};

  register_process(create_kernel_process(fb_proc, "Screen"));
  register_process(create_elf_process("/usr/bin/nomterm", argv, envp));

  dump_pqueue(&ready_queue);

  running = ready_queue.first->pcb;
  switch_to_process(&running->trapframe, (void *)running->cr3);

  return;
}
