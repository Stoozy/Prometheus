#ifndef _TASKING_H
#define _TASKING_H 1

#include "../cpu/cpu.h"
#include "../memory/vmm.h"

enum TaskState {
    CREATED,
	RUNNING,
    READY,
};

typedef struct process_control_block {
    u64 pid;
    char name[256];
    void * p_stack;
	PageTable * cr3;	

    enum TaskState state;

	struct process_control_block * next;	
} ProcessControlBlock;


void multitasking_init();
void _kill(void);
void dump_list();
void schedule();

ProcessControlBlock * create_process(void (void));
void register_process(ProcessControlBlock *);

#endif
