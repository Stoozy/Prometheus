#include "cpu/cpu.h"
#include "memory/vmm.h"
#include <config.h>
#include <drivers/video.h>
#include <fs/vfs.h>
#include <kmalloc.h>
#include <kprintf.h>
#include <memory/pmm.h>
#include <proc/elf.h>
#include <proc/proc.h>
#include <stdint.h>
#include <string/string.h>

extern u64 g_ticks;
extern void switch_to_process(void *new_stack, PageTable *cr3);
extern void load_pagedir();

volatile ProcessQueue ready_queue = {0, NULL, NULL};
volatile ProcessQueue wait_queue = {0, NULL, NULL};
volatile ProcessControlBlock *running = NULL;

PageTable *kernel_cr3 = NULL;

void pqueue_insert(ProcessQueue *queue, ProcessControlBlock *proc) {
  if (queue->first == NULL)
    queue->first = queue->last = proc;

  // append to last
  queue->last->next = proc;
  queue->last = queue->last->next;
  queue->last->next = NULL;

  ++queue->count;
}

void pqueue_remove(ProcessQueue *queue, ProcessControlBlock *proc) {
  kprintf("Removing process at %x\n", proc);
  if (queue->first == proc) {
    if (queue->last == proc) {
      queue->first = queue->last = NULL;
      --queue->count;
      proc->next = NULL;
      return;
    }

    queue->first = queue->first->next;
    --queue->count;
    proc->next = NULL;
    return;
  }

  ProcessControlBlock *cproc = queue->first;
  while (cproc) {
    if (cproc->next == proc) {
      cproc->next = proc->next;
      --queue->count;
      proc->next = NULL;
      return;
    }
    cproc = cproc->next;
  }

  return;
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

void kill_proc(ProcessControlBlock *proc) {
  // TODO
}

void kill_current_proc(void) { kill_proc(running); }

void task_a() {
  extern uint8_t kbd_read_from_buffer();
  for (;;) {
    kprintf("Running task A ...\n");
    // uint8_t c = kbd_read_from_buffer();
    // kprintf("Got character %c\n", c);
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

void dump_queue(ProcessQueue *pqueue) {
  ProcessControlBlock *cproc = pqueue->first;
  kprintf("There are %d processes in this queue\n", pqueue->count);
  for (; cproc; cproc = cproc->next) {
    kprintf("Process at %x\n", cproc);
  }
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

ProcessControlBlock *create_kernel_process(void (*entry)(void)) {

  ProcessControlBlock *pcb = kmalloc(sizeof(ProcessControlBlock));
  memset(pcb, 0, sizeof(ProcessControlBlock));

  void *stack_ptr = (void *)pmm_alloc_blocks(8) + 8 * PAGE_SIZE;

  memset(&pcb->trapframe, 0, sizeof(Registers));

  pcb->trapframe.ss = 0x10;
  pcb->trapframe.rsp = (uint64_t)stack_ptr;
  pcb->trapframe.rflags = 0x202;
  pcb->trapframe.cs = 0x08;
  pcb->trapframe.rip = (uint64_t)entry;

  pcb->cr3 = vmm_get_current_cr3(); // kernel cr3
  pcb->state = READY;
  pcb->next = NULL;

  return pcb;
}

ProcessControlBlock *clone_process(ProcessControlBlock *proc, Registers *regs) {

  ProcessControlBlock *clone = kmalloc(sizeof(ProcessControlBlock));
  memset(clone, 0, sizeof(ProcessControlBlock));
  memcpy(clone, proc, sizeof(ProcessControlBlock));

  for (int i = 0; i < proc->fd_length; i++) {
    File *copy_file = kmalloc(sizeof(File));
    *copy_file = *(proc->fd_table[i]);
    clone->fd_table[i] = copy_file;
  }

  // reset vas so that proper phys addrs get put by vmm_copy_vas
  clone->vas = NULL;
  clone->next = NULL;

  vmm_copy_vas(clone, proc);
  clone->pid++;
  clone->trapframe = *regs;
  clone->mmap_base = proc->mmap_base;

  kprintf("Registers are at stack %x\n", regs);
  kprintf("Fork'd process has cr3: %x\n", clone->cr3);

  return clone;
}

void register_process(ProcessControlBlock *new_pcb) {
  pqueue_insert(&ready_queue, new_pcb);
  return;
}

void block_process(ProcessControlBlock *proc, int reason) {
  // scheduler will handle the rest
  // note: the rest isn't done here
  // because the registers have to
  // be saved once the interrupt fires
  proc->state = reason;
  return;
}

void unblock_process(ProcessControlBlock *proc) {
  proc->state = READY;
  pqueue_remove(&wait_queue, proc);
  pqueue_insert(&ready_queue, proc);
  return;
}

void multitasking_init() {
  // save kernel page tables
  kernel_cr3 = vmm_get_current_cr3();

  memset(&ready_queue, 0, sizeof(ProcessQueue));
  memset(&wait_queue, 0, sizeof(ProcessQueue));

  char *envp[5] = {"SHELL=/usr/bin/bash", "PATH=/usr/bin", "HOME=/",
                   "TERM=linux", NULL};
  char *argvp[3] = {NULL};

  // ProcessControlBlock *nomterm =
  //     create_elf_process("/usr/bin/nomterm", argvp, envp);
  // kprintf("Got process at %x\n", nomterm);
  // register_process(nomterm);

  register_process(create_kernel_process(idle_task));
  register_process(create_kernel_process(task_b));
  register_process(create_kernel_process(task_a));

  // extern void terminal_main();
  // ProcessControlBlock *kterm = create_kernel_process(terminal_main);
  // register_process(kterm);

  // extern void refresh_screen_proc();
  // ProcessControlBlock *video_refresh =
  //     create_kernel_process(refresh_screen_proc);
  // register_process(video_refresh);

  running = ready_queue.first;
  kprintf("Ready queue has %d procs\n", ready_queue.count);
  dump_queue(&ready_queue);
  dump_queue(&wait_queue);

  kprintf("switching to process pid:%d\n", running->pid);
  switch_to_process(&running->trapframe, (void *)running->cr3);
  return;
}
