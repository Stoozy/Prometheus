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

// Creates a process with given entrypoint
// this requires allocating a stack, setting the stack
// and creating page tables
ProcessControlBlock *create_process(void (*entry)(void)) {
  ProcessControlBlock *pcb = kmalloc(sizeof(ProcessControlBlock));

  memset(pcb, 0, sizeof(ProcessControlBlock));

  pcb->p_stack = pmm_alloc_block() + PAGE_SIZE;

  u64 *stack = (u64 *)(pcb->p_stack);
  PageTable *pml4 = vmm_create_user_proc_pml4(stack);

  // user proc
  *--stack = 0x23;              // ss
  *--stack = (u64)pcb->p_stack; // rsp
  *--stack = 0x202;             // rflags
  *--stack = 0x2b;              // cs
  *--stack = (u64)entry;        // rip

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
  pcb->cr3 = pml4;
  pcb->next = NULL;

  return pcb;
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
void register_process(ProcessControlBlock *new_pcb) {

#ifdef SCHEDULER_DEBUG
  kprintf("[TASKING]  Registering process at 0x%x\n", new_pcb);
#endif

  volatile ProcessControlBlock *current_pcb = gp_process_queue;

  // first process
  if (current_pcb == NULL) {
    gp_process_queue = new_pcb;
    ++g_procs;
    return;
  }

  while (current_pcb->next != NULL)
    current_pcb = current_pcb->next;
  current_pcb->next = new_pcb;

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

  extern void terminal_main();
  ProcessControlBlock *term_proc = create_kernel_process(terminal_main);
  register_process(term_proc);

  char *envp[4] = {"PATH=/usr/bin", "HOME=/", "TERM=linux", NULL};
  char *argvp[2] = {"--version", NULL};

  ProcessControlBlock *hello_proc =
      create_elf_process("/usr/bin/bash", envp, argvp);
  register_process(hello_proc);

  gp_current_process = gp_process_queue;

  // kprintf("Switching to process with 0x%llx cr3\n",
  // (void *)gp_current_process->cr3);
  switch_to_process(gp_current_process->p_stack,
                    (void *)gp_current_process->cr3);
  return;
}
