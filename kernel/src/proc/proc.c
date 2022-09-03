#include "cpu/cpu.h"
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

ProcessControlBlock *gp_process_queue = NULL;
ProcessControlBlock *gp_current_process = NULL;
PageTable *kernel_cr3 = NULL;
u64 g_procs = 0;

extern u64 g_ticks;
extern void switch_to_process(void *new_stack, PageTable *cr3);
extern void load_pagedir();

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

  // Need kernel cr3 for kernel data structures...
  load_pagedir(kernel_cr3);

  ProcessControlBlock *pcb = gp_process_queue;

  while (pcb->next != gp_current_process)
    pcb = pcb->next;

  // pcb->next now points to gp_current_process

  ProcessControlBlock *node_to_remove = pcb->next;

  // skip a node, and relink
  pcb->next = gp_current_process->next;

  // TODO: free memory

  // decrease global number of procs
  --g_procs;
}

void kill_current_proc(void) { kill_proc(gp_current_process); }

void task_a() {
  for (;;)
    kprintf("Running task A...\n");
}

void task_b() {
  for (;;)
    kprintf("Running task B...\n");
}
void idle_task() {
  for (;;)
    kprintf("Idling...\n");
}

void dump_list() {
  kprintf("---------PROCESS QUEUE---------\n");
  volatile ProcessControlBlock *current = gp_process_queue;

  kprintf("0x%x ->", current);
  while (current->next != NULL) {
    current = current->next;
    kprintf("0x%x ->", current);
  }
  kprintf(" NULL\n");
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

    // copy_file->name = kmalloc(strlen(proc->fd_table[i]->name) + 1);
    // strcpy(copy_file->name, proc->fd_table[i]->name);
    // copy_file->name[strlen(proc->fd_table[i]->name)] = '\0';

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

#ifdef SCHEDULER_DEBUG
  kprintf("[TASKING]  Registering process at 0x%x\n", new_pcb);
#endif

  volatile ProcessControlBlock *current_pcb = gp_process_queue;

  kprintf("Current pcb is at %x\n", current_pcb);

  // first process
  if (!current_pcb) {
    gp_process_queue = new_pcb;
    ++g_procs;
    return;
  }

  while (current_pcb->next != NULL)
    current_pcb = current_pcb->next;
  current_pcb->next = new_pcb;
  new_pcb->next = NULL;

#ifdef SCHEDULER_DEBUG
  kprintf("[TASKING]  PCB at 0x%x is after 0x%x\n", current_pcb->next,
          current_pcb);
  kprintf("[TASKING]  PCB has next 0x%x\n", current_pcb->next);
#endif

  ++g_procs;
  return;
}

void multitasking_init() {
  // save kernel page tables
  kernel_cr3 = vmm_get_current_cr3();

  char *envp[5] = {"SHELL=/usr/bin/bash", "PATH=/usr/bin", "HOME=/",
                   "TERM=linux", NULL};
  char *argvp[3] = {NULL};

  // ProcessControlBlock *nomterm =
  //     create_elf_process("/usr/bin/nomterm", argvp, envp);
  // kprintf("Got process at %x\n", nomterm);
  // register_process(nomterm);

  extern void refresh_screen_proc();
  ProcessControlBlock *video_refresh =
      create_kernel_process(refresh_screen_proc);
  register_process(video_refresh);
  gp_current_process = gp_process_queue;

  kprintf("switching to process pid:%d\n", gp_current_process->pid);
  switch_to_process(&gp_current_process->trapframe,
                    (void *)gp_current_process->cr3);
  return;
}
