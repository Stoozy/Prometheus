#include "tasking.h"
#include "../memory/vmm.h"
#include "../memory/pmm.h"
#include "../string/string.h"
#include "../kprintf.h"
#include "../kmalloc.h"


TaskControlBlock * gp_task_queue;
TaskControlBlock * gp_current_task;

extern void load_pagedir(void * cr3);
extern u64 g_ticks;

volatile u64 tasks;

static PageTable * get_current_cr3(){
    PageTable * current_cr3;
    asm volatile (" mov %%cr3, %0" : "=r"(current_cr3));
    return current_cr3;
}


void scheduler(Registers regs){
    if(tasks == 0) return;

    kprintf("[SCHEDULER]    Global ticks: %llu\n", g_ticks);

    // save registers
    memcpy((void*)&gp_current_task->cpu_registers, &regs, sizeof(Registers));

    if(g_ticks % 30 == 0){
        if(gp_current_task->next != NULL)
            gp_current_task = gp_current_task->next;
        else gp_current_task = gp_task_queue;
    }

    // set registers
    memcpy((void*)&regs, &gp_current_task->cpu_registers, sizeof(Registers));
    load_pagedir(gp_current_task->cr3);
    return;
}

void task_a(){
    for(;;){
        kprintf("Running task A\n");
    }
}

void create_task(void (*entrypoint)(void)){

    kprintf("[SCHEDULER]    Called create task with 0x%x\n", entrypoint);

    TaskControlBlock * tcb = kmalloc(sizeof(TaskControlBlock));
    //TaskControlBlock * tcb = kmalloc(sizeof(TaskControlBlock));

    tcb->cr3 = vmm_create_user_proc_pml4();
    tcb->next = NULL;

    /* append task */
    
    
    TaskControlBlock * current_task = gp_current_task;
    if(tasks != 0){
        while(current_task->next != NULL){
            current_task = current_task->next;
        }
        current_task->next = tcb;
    }else{
        // first task created
        gp_current_task = tcb;
    }
    
    ++tasks;

    kprintf("[SCHEDULER]    Created task\n");
    load_pagedir(tcb->cr3);

}

void multitasking_init(){

    tasks = 0;
    create_task(task_a);

}




