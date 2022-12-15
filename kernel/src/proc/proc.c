#include "cpu/cpu.h"
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
extern void switch_to_process(void *new_stack, PageTable *cr3);
extern void load_pagedir();


ProcessQueue ready_queue = {0, NULL, NULL};
ProcessQueue wait_queue = {0, NULL, NULL};
ProcessControlBlock *running = NULL;

PageTable *kernel_cr3 = NULL;

uint64_t pid_counter = 200;

void dump_pqueue(ProcessQueue *pqueue) {
  ProcessControlBlock *cproc = pqueue->first;
  kprintf("There are %d processes in this queue\n", pqueue->count);
  kprintf("Head: %x; Tail: %x;\n", pqueue->first, pqueue->last);
  for (; cproc; cproc = cproc->next) {
    kprintf("Process  %d ; Next %x\n", cproc->pid, cproc->next);
  }
}

void pqueue_insert(ProcessQueue *queue, ProcessControlBlock *proc) {

  if (!proc)
    return;

  if (queue->first == NULL) {
    queue->first = queue->last = proc;
    ++queue->count;
    return;
  }

  // append to last
  queue->last->next = proc;
  queue->last = queue->last->next;
  ++queue->count;
}

void pqueue_remove(ProcessQueue *queue, ProcessControlBlock *proc) {
  kprintf("Removing process at %x\n", proc);

  // TODO

  if (proc == queue->first) {
    if (queue->first->next)
      queue->first = queue->first->next;
    else
      queue->first = queue->last = NULL;

    --queue->count;
    return;
  }

  ProcessControlBlock *cproc = queue->first;
  ProcessControlBlock *nproc = cproc->next;

  while (nproc->next) {
    if (nproc == proc) {
      cproc->next = proc->next;
      --queue->count;
      return;
    }

    nproc = nproc->next;
    cproc = cproc->next;
  }

  // last item
  if (nproc == proc) {
    cproc->next = NULL;
    queue->last = cproc;
    --queue->count;
    return;
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
    
    kprintf("Before removing\n");
    dump_pqueue(&ready_queue);
    pqueue_remove(&ready_queue, proc);
    kprintf("After removing\n");
    dump_pqueue(&ready_queue);

    __asm__("sti");
    for(;;);
}

void kill_cur_proc() { kill_proc(running); }

void task_a() {
  extern uint8_t kbd_read_from_buffer();
  for (;;) {
    kprintf("Running task A ...\n");
    uint8_t c = kbd_read_from_buffer();
    for (;;)
      kprintf("Task A woke up, got character %c \n", c);
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
  pcb->pid = pid_counter++;
  pcb->next = NULL;

  return pcb;
}

ProcessControlBlock *clone_process(ProcessControlBlock *proc, Registers *regs) {

  ProcessControlBlock *clone = kmalloc(sizeof(ProcessControlBlock));
  memset(clone, 0, sizeof(ProcessControlBlock));
  memcpy(clone, proc, sizeof(ProcessControlBlock));

  // for (int i = 0; i < proc->fd_length; i++) {
  //   File *copy_file = kmalloc(sizeof(File));
  //   *copy_file = *(proc->fd_table[i]);
  //   clone->fd_table[i] = copy_file;
  // }

  // reset vas so that proper phys addrs get put by vmm_copy_vas
  clone->vas = NULL;
  clone->next = NULL;

  vmm_copy_vas(clone, proc);
  clone->pid = pid_counter++;
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
  kprintf("Blocking process at %x\n", proc);
  return;
}

void unblock_process(ProcessControlBlock *proc) {
  proc->state = READY;
  proc->next = NULL;
  pqueue_insert(&ready_queue, proc);
  kprintf("\n unblocked %x\n\n", proc);
  return;
}

void multitasking_init() {
  // save kernel page tables
  kernel_cr3 = vmm_get_current_cr3();

  memset(&ready_queue, 0, sizeof(ProcessQueue));
  memset(&wait_queue, 0, sizeof(ProcessQueue));

  char *envp[5] = {"SHELL=/usr/bin/bash", "PATH=/usr/bin", "HOME=/",
                   "TERM=linux", NULL};
  char *argvp[1] = {NULL};

  // ProcessControlBlock *nomterm =
  //     create_elf_process("/usr/bin/nomterm", argvp, envp);
  // kprintf("Got process at %x\n", nomterm);
  // register_process(nomterm);

  extern void refresh_screen_proc();
  extern void terminal_main();

  // register_process(create_kernel_process(task_a));
  // register_process(create_kernel_process(idle_task));
  // register_process(create_kernel_process(task_b));

  register_process(create_kernel_process(terminal_main));
  ProcessControlBlock *bash = create_elf_process("/usr/bin/bash", argvp, envp);
  register_process(bash);
  register_process(create_kernel_process(refresh_screen_proc));

  running = ready_queue.first;
  kprintf("Ready queue has %d procs\n", ready_queue.count);
  dump_pqueue(&ready_queue);

  dump_regs(&ready_queue.first->trapframe);
  kprintf("switching to process pid:%d\n", running->pid);
  switch_to_process(&running->trapframe, (void *)running->cr3);
  return;
}
