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
#include <proc/scheduler.h>
#include <stdint.h>
#include <string/string.h>

PageTable *kernel_cr3 = NULL;
u64 g_procs = 0;

extern u64 g_ticks;
extern void switch_to_process(void *new_stack, PageTable *cr3);
extern void load_pagedir();

/* in scheduler.c */
extern ProcessControlBlock *gp_current_process;

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

  void *stack_ptr = (void *)pmm_alloc_block() + PAGE_SIZE;

  memset(&pcb->trapframe, 0, sizeof(Registers));

  pcb->trapframe.ss = 0x10;
  pcb->trapframe.rsp = (uint64_t)stack_ptr;
  pcb->trapframe.rflags = 0x202;
  pcb->trapframe.cs = 0x08;
  pcb->trapframe.rip = (uint64_t)entry;

  pcb->cr3 = vmm_get_current_cr3(); // kernel cr3
  pcb->state = READY;

  return pcb;
}

ProcessControlBlock *clone_process(ProcessControlBlock *proc, Registers *regs) {

  ProcessControlBlock *clone = kmalloc(sizeof(ProcessControlBlock));
  memset(clone, 0, sizeof(ProcessControlBlock));
  memcpy(clone, proc, sizeof(ProcessControlBlock));

  for (int i = 0; i < MAX_PROC_FDS; i++) {
    if (proc->fd_table[i]) {
      kprintf("copying fd %d\n", i);
      File *copy_file = kmalloc(sizeof(File));
      *copy_file = *(proc->fd_table[i]);
      clone->fd_table[i] = copy_file;
    }
  }

  // reset vas so that proper phys addrs get put by vmm_copy_vas
  clone->vas = NULL;

  vmm_copy_vas(clone, proc);
  clone->pid++;
  clone->trapframe = *regs;
  clone->mmap_base = MMAP_BASE;

  kprintf("Registers are at stack %x\n", regs);
  kprintf("Fork'd process has cr3: %x\n", clone->cr3);

  return clone;
}

void multitasking_init() {
  scheduler_init();
  // save kernel page tables
  kernel_cr3 = vmm_get_current_cr3();

  char *envp[4] = {"PATH=/usr/bin", "HOME=/", "TERM=linux", NULL};
  char *argvp[2] = {NULL};

  extern void refresh_screen_proc();
  ProcessControlBlock *video_refresh =
      create_kernel_process(refresh_screen_proc);
  enqueue_process(video_refresh);

  ProcessControlBlock *fbpad =
      create_elf_process("/usr/bin/fbpad", argvp, envp);
  kprintf("Got process at %x\n", fbpad);
  enqueue_process(fbpad);
  // dump_process_queue();
  asm("sti");
  //   kprintf("switching to process pid:%d\n", gp_current_process->pid);
  //   switch_to_process(&gp_current_process->trapframe,
  //(void *)gp_current_process->cr3);
  return;
}
