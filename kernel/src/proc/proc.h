#pragma once

#include <stdint.h>

#include "../cpu/cpu.h"
#include "../memory/vmm.h"
#include "../fs/vfs.h"
#include "../config.h"

enum TaskState {
    READY,
	RUNNING,
    ZOMBIE
};

typedef struct process_control_block {
    uint64_t pid;
    char name[256];
    void * p_stack;
	PageTable * cr3;	
    enum TaskState state;

    struct file * fd_table[MAX_PROC_FDS];
    int fd_length;

	struct process_control_block * next;	
} ProcessControlBlock;


void map_fd_to_proc(ProcessControlBlock * proc, struct file  *node);
void multitasking_init();
void kill_current_proc(void);
void dump_list();
void schedule();

ProcessControlBlock * create_process(void (void));
void register_process(ProcessControlBlock *);

