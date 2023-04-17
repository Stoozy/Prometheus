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
#include <sys/queue.h>

extern u64 g_ticks;
extern void switch_to_process(Registers *new_stack, PageTable *cr3);
extern void load_pagedir();

ProcessControlBlock *running = NULL;

uint64_t pid_counter = 200;

struct procq readyq;

void dump_readyq() {
  ProcessControlBlock *cur;
  TAILQ_FOREACH(cur, &readyq, entries) {
    kprintf("%s (%d) -> ", cur->name, cur->pid);
  }
  kprintf(" None");
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
  dump_readyq();

  TAILQ_REMOVE(&readyq, proc, entries);

  proc->exit_code = exit_code;
  proc->state = ZOMBIE;

  proc->exit_code = exit_code;

  if (proc->parent) {
    proc->parent->childDied = true;
  }

  kprintf("After removing\n");
  dump_readyq();

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

  TAILQ_INIT(&clone->children);

  TAILQ_INSERT_TAIL(&proc->children, clone, child_entries);

  clone->parent = proc;

  return clone;
}

void register_process(ProcessControlBlock *new) {
  TAILQ_INSERT_TAIL(&readyq, new, entries);
  return;
}

void multitasking_init() {

  TAILQ_INIT(&readyq);

  char *argv[2] = {"/usr/bin/gcon", NULL};
  char *envp[3] = {"PATH=/usr/bin", NULL};

  extern void fb_proc();
  register_process(create_kernel_process(fb_proc, "Screen"));
  register_process(create_elf_process("/usr/bin/gcon", argv, envp));

  dump_readyq();

  running = TAILQ_FIRST(&readyq);
  switch_to_process(&running->trapframe, (void *)running->cr3);
  return;
}
