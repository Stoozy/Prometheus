#pragma once

#include <stdint.h>

#include <config.h>
#include <cpu/cpu.h>
#include <fs/vfs.h>
#include <memory/vmm.h>
#include <proc/proc.h>

#define STACK_BLOCKS 16
#define STACK_SIZE STACK_BLOCKS *PAGE_SIZE
#define MMAP_BASE 0xC000000000

enum TaskState { READY, RUNNING, ZOMBIE, WAITING };

struct pqueue;

typedef struct vas_range_node VASRangeNode;

struct process_control_block;

typedef struct pqueue {
    int count;
    struct process_control_block *first;
    struct process_control_block *last;
} ProcessQueue;

typedef struct process_control_block {
    uint64_t pid;
    char name[256];

    Registers trapframe;
    PageTable *cr3;

    enum TaskState state;

    VASRangeNode *vas;
    uint64_t mmap_base;

    struct file *fd_table[MAX_PROC_FDS];
    int fd_length;

    int exit_code;
    bool childDied;
    ProcessQueue children;

    struct process_control_block *parent;
    struct process_control_block *next;
} ProcessControlBlock;

void block_process(ProcessControlBlock *, int);
void unblock_process(ProcessControlBlock *);

void pqueue_push(ProcessQueue *, ProcessControlBlock *);
ProcessControlBlock *pqueue_pop(ProcessQueue *);
void pqueue_remove(ProcessQueue *, int pid);

void unmap_fd_from_proc(ProcessControlBlock *proc, int fd);
int map_file_to_proc(ProcessControlBlock *proc, struct file *file);

void kill_current_proc(void);
void dump_list();
void dump_proc_vas(ProcessControlBlock *);
void multitasking_init();

void proc_add_vas_range(ProcessControlBlock *, VASRangeNode *);

ProcessControlBlock *create_process(void(void));
ProcessControlBlock *clone_process(ProcessControlBlock *proc, Registers *regs);

ProcessControlBlock *get_next_ready_process();

void register_process(ProcessControlBlock *);

void schedule(Registers *);
