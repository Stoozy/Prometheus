#pragma once

#include <stdint.h>

#include <config.h>
#include <cpu/cpu.h>
#include <fs/vfs.h>
#include <memory/vmm.h>

enum TaskState { READY, RUNNING, ZOMBIE };

typedef struct process_control_block {
  uint64_t pid;
  char name[256];
  void *p_stack;
  PageTable *cr3;
  enum TaskState state;

  uint64_t mmap_base;

  struct file *fd_table[MAX_PROC_FDS];
  int fd_length;

  struct process_control_block *next;
} ProcessControlBlock;

void unmap_fd_from_proc(ProcessControlBlock *proc, int fd);
int map_file_to_proc(ProcessControlBlock *proc, struct file *file);
void multitasking_init();
void kill_current_proc(void);
void dump_list();
void schedule();

ProcessControlBlock *create_process(void(void));
void register_process(ProcessControlBlock *);
