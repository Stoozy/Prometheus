#include <config.h>
#include <drivers/video.h>
#include <fs/vfs.h>
#include <kmalloc.h>
#include <kprintf.h>
#include <memory/pmm.h>
#include <proc/elf.h>
#include <proc/proc.h>
#include <string/string.h>

ProcessControlBlock *gp_process_queue = NULL;
ProcessControlBlock *gp_current_process = NULL;

extern void switch_to_process(void *new_stack, PageTable *cr3);

extern void load_pagedir();
extern u64 g_ticks;

u64 g_procs = 0;

PageTable *kernel_cr3;

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

  // free memory
  pmm_free_block((u64)node_to_remove->p_stack);
  pmm_free_block((u64)node_to_remove);

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

  pcb->p_stack = pmm_alloc_block() + PAGE_SIZE;

  u64 *stack = (u64 *)(pcb->p_stack);

  // user proc
  *--stack = 0x10;              // ss
  *--stack = (u64)pcb->p_stack; // rsp
  *--stack = 0x202;             // rflags
  *--stack = 0x08;              // cs
  *--stack = (uintptr_t)entry;  // rip

  *--stack = 0; // r8
  *--stack = 0;
  *--stack = 0;
  *--stack = 0; // ...
  *--stack = 0;
  *--stack = 0;
  *--stack = 0;
  *--stack = 0; // r15

  *--stack = 0; // rbp

  *--stack = 0; // rdx
  *--stack = 0; // rcx
  *--stack = 0; // rbx
  *--stack = 0; // rax
  *--stack = 0; // rsi
  *--stack = 0; // rdi

  pcb->p_stack = stack;
  pcb->cr3 = vmm_get_current_cr3(); // kernel cr3
  pcb->next = NULL;

  return pcb;
}

ProcessControlBlock *clone_process(ProcessControlBlock *proc, Registers *regs) {

  ProcessControlBlock *clone = kmalloc(sizeof(ProcessControlBlock));
  memset(clone, 0, sizeof(ProcessControlBlock));
  memcpy(clone, proc, sizeof(ProcessControlBlock));

  clone->pid++;
  clone->cr3 = vmm_copy_vas(proc);
  clone->p_stack = regs;
  kprintf("Registers are at stack %x\n", regs);

  kprintf("Fork'd process has cr3: %x\n", clone->cr3);

  clone->next = NULL;

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

  char *envp[4] = {"PATH=/usr/bin", "HOME=/", "TERM=linux", NULL};
  char *argvp[2] = {NULL};

  ProcessControlBlock *fbpad =
      create_elf_process("/usr/bin/fork_test", argvp, envp);
  kprintf("Got process at %x\n", fbpad);
  register_process(fbpad);

  // extern void refresh_screen_proc();
  // ProcessControlBlock *video_refresh =
  //     create_kernel_process(refresh_screen_proc);
  // register_process(video_refresh);

  gp_current_process = gp_process_queue;

  kprintf("switching to process pid:%d\n", gp_current_process->pid);
  switch_to_process(gp_current_process->p_stack,
                    (void *)gp_current_process->cr3);
  return;
}
