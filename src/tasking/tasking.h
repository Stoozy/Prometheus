#ifndef _TASKING_H
#define _TASKING_H 1

#include "../cpu/cpu.h"
#include "../memory/vmm.h"

enum TaskState {
	RUNNING,
	READY_TO_RUN
};

typedef struct task_control_block {
	Registers cpu_registers;    
	PageTable * cr3;	
	enum TaskState task_state;

	struct task_control_block * next;	
} TaskControlBlock;


void multitasking_init();
void scheduler(Registers regs);

#endif
