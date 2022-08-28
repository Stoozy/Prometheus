#pragma once
#include "memory/vmm.h"
#include <proc/proc.h>

typedef struct pcb_node {
    ProcessControlBlock * pcb;
    struct pcb_node *next;
} PCBNode;

typedef struct process_queue {
    int count;
    PCBNode * front;
    PCBNode * rear;
} ProcessQueue;

void dump_process_queue();

void enqueue_process(ProcessControlBlock*);
void dequeue_process();

void block_process(ProcessControlBlock*);
void unblock_process(ProcessControlBlock*);

void schedule(Registers *regs);

void scheduler_init();
