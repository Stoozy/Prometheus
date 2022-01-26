#pragma once

#include <stdint.h>

#include "../cpu/cpu.h"
#include "../memory/vmm.h"
#include "../fs/vfs.h"

enum TaskState {
    READY,
	RUNNING,
    ZOMBIE
};

typedef struct fd_table{
    VfsNode * entries;
    int length;
} FdTable;

typedef struct process_control_block {
    uint64_t pid;
    char name[256];
    void * p_stack;
	PageTable * cr3;	
    enum TaskState state;
    FdTable fd_table;

	struct process_control_block * next;	
} ProcessControlBlock;


void multitasking_init();
void kill_current_proc(void);
void dump_list();
void schedule();

ProcessControlBlock * create_process(void (void));
void register_process(ProcessControlBlock *);

