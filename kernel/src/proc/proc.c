#include "../config.h"
#include "proc.h"
#include "../memory/pmm.h"
#include "../kprintf.h"
#include "../kmalloc.h"
#include "../string/string.h"
#include "../drivers//video.h"

volatile ProcessControlBlock * gp_process_queue;
volatile ProcessControlBlock * gp_current_process;

extern void switch_to_process(void * new_stack, PageTable * cr3);
extern void switch_to_user_proc( void * instruction_ptr, void * stack);
extern void load_pagedir();
extern u64  g_ticks;

volatile u64 g_procs;


void kill_proc(ProcessControlBlock * proc){
    ProcessControlBlock * pcb = gp_process_queue;

    while(pcb->next != gp_current_process)
        pcb = pcb->next;

    // pcb->next now points to gp_current_process
    
    ProcessControlBlock * node_to_remove = pcb->next;

    // skip a node, and relink
    pcb->next = gp_current_process->next;

    // free memory
    pmm_free_block((u64)node_to_remove->p_stack); 
    pmm_free_block((u64)node_to_remove);

    // decrease global number of procs
    --g_procs;
}

void kill_current_proc(void){
    kill_proc(gp_current_process);
}

void task_a(){ 
    for(;;)
        kprintf("Running task A...\n");
}

void task_b(){ 
    //for(;;)
        //kprintf("Running task B...\n");

    asm volatile("mov $0, %rdi\n\t\
                syscall");
    
    for(;;) kprintf("Running task B...\n");

}

void task_c(){ 
    for(;;)
        kprintf("Running task C...\n");

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

void save_context(volatile ProcessControlBlock * proc, Registers * regs){

    proc->p_stack = (void*)regs->rsp;

    u64 * stack = proc->p_stack;
    *--stack = regs->ss; // ss
    *--stack = (u64)proc->p_stack; // rsp
    *--stack = regs->rflags ; // rflags
    *--stack = regs->cs; // cs
    *--stack = (u64)regs->rip; // rip

    *--stack = (u64) regs->r8;
    *--stack = (u64) regs->r9;
    *--stack = (u64) regs->r10;
    *--stack = (u64) regs->r11;
    *--stack = (u64) regs->r12;
    *--stack = (u64) regs->r13;
    *--stack = (u64) regs->r14;
    *--stack = (u64) regs->r15;

    *--stack = regs->rbp; // rbp
    *--stack = regs->rbx; // rbx
    *--stack = regs->rsi; // rsi
    *--stack = regs->rdi; // rdi

    proc->p_stack = stack;
    
    // TODO: this should use actual processor ids
    // update LocalCpuData
    LocalCpuData * lcd = get_cpu_struct(0); 

    lcd->regs = *regs;
    lcd->syscall_user_stack = stack;

    return;
}


void schedule(Registers * regs){

#ifdef SCHEDULER_DEBUG
    dump_regs(regs);
    kprintf("[SCHEDULER]    %d Global Processes\n", g_procs);
#endif 
    
    // save current proc 
    save_context(gp_current_process, regs);
    
    // not enough procs or not time to switch yet
    if(g_procs == 0 || g_ticks % SMP_TIMESLICE != 0) 
        return;

    if(gp_current_process->next == NULL) {
        gp_current_process = gp_process_queue; // go to head

#ifdef SCHEDULER_DEBUG
        kprintf("[SCHEDULER] Switching to head:\n");
        dump_regs(gp_current_process->p_stack);
#endif

    } else if (gp_current_process->next != NULL){
        // move to next proc
        gp_current_process = gp_current_process->next;
#ifdef SCHEDULER_DEBUG
        kprintf("[SCHEDULER] Switching to next: \n");
        dump_regs(gp_current_process->p_stack);
#endif

    }
    if(gp_current_process->state == ZOMBIE){
        
    }

    // finally, switch
    switch_to_process(gp_current_process->p_stack, gp_current_process->cr3);

}

ProcessControlBlock * create_process(void (*entry)(void)){
    // TODO: need allocator
    ProcessControlBlock * pcb = pmm_alloc_block();
        /*kmalloc(sizeof(ProcessControlBlock));*/

    memset(pcb, 0, sizeof(ProcessControlBlock));

    pcb->p_stack = pmm_alloc_block()+0x1000;

	u64 * stack = (u64 *)(pcb->p_stack);
    PageTable * pml4 = vmm_create_user_proc_pml4(stack);

    // user proc
	*--stack = 0x23; // ss
	*--stack = (u64)pcb->p_stack; // rsp
	*--stack = 0x202 ; // rflags
	*--stack = 0x2b; // cs
	*--stack = (u64)entry; // rip

    *--stack = 0; // r8
    *--stack = 0;
    *--stack = 0; 
    *--stack = 0; // ...
    *--stack = 0; 
    *--stack = 0;
    *--stack = 0;
    *--stack = 0; // r15


	*--stack = (u64)pcb->p_stack; // rbp

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
    register_process(create_process(task_b));

    register_process(create_process(refresh_screen_proc));
    gp_current_process = gp_process_queue;

    switch_to_process(gp_current_process->p_stack, gp_current_process->cr3);
    asm volatile ("sti");
}
