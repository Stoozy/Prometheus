#ifndef _TASKING_H
#define _TASKING_H 1

#include "../cpu/cpu.h"
#include "../memory/vmm.h"

enum TaskState {
	RUNNING,
	READY_TO_RUN
};

typedef struct task_control_block {
	Registers context;    
	PageTable * cr3;	

	struct task_control_block * next;	
} TaskControlBlock;


void multitasking_init();
volatile void scheduler(Registers * regs);

#endif
