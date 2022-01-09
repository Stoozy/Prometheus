#include "../config.h"
#include "proc.h"
#include "../memory/pmm.h"
#include "../kprintf.h"
#include "../kmalloc.h"
#include "../string/string.h"

volatile ProcessControlBlock * gp_process_queue;
volatile ProcessControlBlock * gp_current_process;

extern void switch_to_process(void * new_stack, PageTable * cr3);
extern void switch_to_user_proc( void * instruction_ptr, void * stack);
extern void load_pagedir();
extern u64  g_ticks;

volatile u64 g_procs;

void _kill(void){
    // remove pcb from list
    // free pcb and allocated memory by elf loader
    // return?

    ProcessControlBlock * next_to_current = gp_current_process;
    ProcessControlBlock * current_pcb = gp_process_queue;

    // relink the nodes
    while(current_pcb->next != gp_current_process)
        current_pcb = current_pcb->next;
    current_pcb->next = next_to_current;

    kfree(gp_current_process);
}


void task_a(){ 
    for(;;){
        kprintf("Running task A...\n");
    }
}

void task_b(){ 
    for(;;){
        kprintf("Running task B...\n");
    }
}

void idle_task(){ 
    for(;;){
        kprintf("Idling...\n");
    }
}

void dump_list(){
    kprintf("---------PROCESS QUEUE---------\n");
    volatile ProcessControlBlock * current = gp_process_queue;

    kprintf("0x%x ->", current);
    while(current->next != NULL){
        current = current->next;
        kprintf("0x%x ->", current);
    }
    kprintf(" NULL\n");
}

void dump_regs(void * stack){
    Registers * regs = (Registers*) stack;
    kprintf("[SCHEDULER]    RIP: 0x%x\n", regs->rip);
    kprintf("[SCHEDULER]    RSP: 0x%x\n", regs->rsp);
    kprintf("[SCHEDULER]    RBP: 0x%x\n", regs->rbp);
    kprintf("[SCHEDULER]    RBX: 0x%x\n", regs->rbx);
    kprintf("[SCHEDULER]    RSI: 0x%x\n", regs->rsi);
    kprintf("[SCHEDULER]    RDI: 0x%x\n", regs->rdi);
    kprintf("[SCHEDULER]    RFLAGS: 0x%x\n", regs->rflags);
    kprintf("[SCHEDULER]    SS: 0x%x\n", regs->ss);
    kprintf("[SCHEDULER]    CS: 0x%x\n", regs->cs);
}

void schedule(Registers * regs){

#ifdef SCHEDULER_DEBUG
    dump_regs(regs);
    kprintf("[SCHEDULER]    %d Global Processes\n", g_procs);
#endif 
    if(g_procs == 0)
        return; // not enough processes

    if(g_ticks % SMP_TIMESLICE != 0) return; // not time to switch yet

    // save current proc 
    gp_current_process->p_stack = (void*)regs->rsp;

    u64 * stack = gp_current_process->p_stack;
    *--stack = regs->ss; // ss
    *--stack = (u64)gp_current_process->p_stack; // rsp
    *--stack = regs->rflags ; // rflags
    *--stack = regs->cs; // cs
    *--stack = (u64)regs->rip; // rip
    *--stack = (u64)regs->rbp; // rbp
    *--stack = 0; // rdx
    *--stack = regs->rsi; // rsi
    *--stack = regs->rdi; // rdi

    gp_current_process->p_stack = stack;

    // reached tail, go to head
    if(gp_current_process->next == NULL){

        // switch to head
        gp_current_process = gp_process_queue;

#ifdef SCHEDULER_DEBUG
        kprintf("[SCHEDULER] Switching to head:\n");
        dump_regs(gp_current_process->p_stack);
#endif
        //Registers * regs = gp_current_process->p_stack;

        //u64 cs =  0, cr3 = 0;
        //asm volatile (" mov %%cs, %0" : "=r"(cs));
        //asm volatile (" mov %%cr3, %0" : "=r"(cr3));

        //if(cr3 != (u64)vmm_get_current_cr3()){
        //    load_pagedir(cr3);
        //}        if(cs == 0x2b){
        //    switch_to_user_proc((void*)regs->rip, (void*)regs->rsp);
        //}else{
            switch_to_process(gp_current_process->p_stack, gp_current_process->cr3);
        //}

    }else{

        // just go to next process 
        gp_current_process = gp_current_process->next;

#ifdef SCHEDULER_DEBUG
        kprintf("[SCHEDULER] Switching to next: \n");
        dump_regs(gp_current_process->p_stack);
#endif
        //Registers * regs = gp_current_process->p_stack;

        //u64 cs =  0, cr3 = 0;
        //asm volatile (" mov %%cs, %0" : "=r"(cs));
        //asm volatile (" mov %%cr3, %0" : "=r"(cr3));

        //if(cr3 != (u64)vmm_get_current_cr3()){
        //    load_pagedir(cr3);
        //}

        //if(cs == 0x2b){
        //    switch_to_user_proc((void*)regs->rip, (void*)regs->rsp);
        //}else{
            switch_to_process(gp_current_process->p_stack, gp_current_process->cr3);
        //}
    }

}

ProcessControlBlock * create_process(void (*entry)(void)){
    ProcessControlBlock * pcb = pmm_alloc_block();
        /*kmalloc(sizeof(ProcessControlBlock));*/

    pcb->p_stack = pmm_alloc_block();

	u64 * stack = (u64 *)(pcb->p_stack);
    PageTable * pml4 = vmm_create_user_proc_pml4(stack);

    // user proc
	*--stack = 0x23; // ss
	*--stack = (u64)pcb->p_stack; // rsp
	*--stack = 0x202 ; // rflags
	*--stack = 0x2b; // cs
	*--stack = (u64)entry; // rip
	*--stack = (u64)pcb->p_stack; //ebp
	*--stack = 0; // rdx
	*--stack = 0; // rsi
	*--stack = 0; // rdi

    pcb->p_stack = stack;

    pcb->cr3 = pml4; 
    pcb->next = NULL;

    return pcb; 
}

void register_process(ProcessControlBlock * new_pcb){

#ifdef SCHEDULER_DEBUG
    kprintf("[TASKING]  Registering process at 0x%x\n", new_pcb);
#endif

    volatile ProcessControlBlock * current_pcb = gp_process_queue;  

    // first process
    if(current_pcb == NULL){
        gp_process_queue = new_pcb;
        ++g_procs;
        return;
    }

    while(current_pcb->next != NULL)
        current_pcb = current_pcb->next;
    current_pcb->next = new_pcb;

#ifdef SCHEDULER_DEBUG
    kprintf("[TASKING]  PCB at 0x%x is after 0x%x\n", current_pcb->next, current_pcb);
    kprintf("[TASKING]  PCB has next 0x%x\n", current_pcb->next);
#endif

    ++g_procs;

}

void multitasking_init(){
    gp_process_queue = NULL;
    gp_current_process = NULL;

    g_procs = 0;

    register_process(create_process(idle_task));
    register_process(create_process(task_a));
    register_process(create_process(task_b));
    gp_current_process = gp_process_queue;
}
