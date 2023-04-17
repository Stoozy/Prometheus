#pragma once

#include <stdint.h>

#include <config.h>
#include <cpu/cpu.h>
#include <fs/vfs.h>
#include <memory/vmm.h>
#include <proc/proc.h>

#define STACK_BLOCKS 32
#define STACK_SIZE STACK_BLOCKS *PAGE_SIZE
#define MMAP_BASE 0xC000000000

enum TaskState { READY, RUNNING, ZOMBIE, WAITING };

typedef struct vas_range_node VASRangeNode;
struct process_control_block;

TAILQ_HEAD(procq, process_control_block);

typedef struct process_control_block {
  uint64_t pid;
  char name[256];

  char *cwd;

  uintptr_t fs_base;
  Registers trapframe;
  void *kstack;
  PageTable *cr3;

  enum TaskState state;

  VASRangeNode *vas;
  uint64_t mmap_base;

  struct file *fd_table[MAX_PROC_FDS];
  int fd_length;

  TAILQ_ENTRY(process_control_block) entries;

  TAILQ_ENTRY(process_control_block) child_entries;
  struct procq children;

  int exit_code;
  bool childDied;

  struct process_control_block *parent;

} ProcessControlBlock;

void block_process(ProcessControlBlock *, int);
void unblock_process(ProcessControlBlock *);

void unmap_fd_from_proc(ProcessControlBlock *proc, int fd);
int map_file_to_proc(ProcessControlBlock *proc, struct file *file);

void kill_current_proc(void);
void dump_readyq();
void dump_proc_vas(ProcessControlBlock *);
void multitasking_init();

void proc_add_vas_range(ProcessControlBlock *, VASRangeNode *);

ProcessControlBlock *create_process(void(void));
ProcessControlBlock *clone_process(ProcessControlBlock *proc, Registers *regs);

ProcessControlBlock *get_next_ready_process();

void register_process(ProcessControlBlock *);

void schedule(Registers *);

extern struct procq readyq;
extern PageTable *kernel_cr3;
