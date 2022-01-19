#include "../config.h"
#include "proc.h"
#include "../memory/pmm.h"
#include "../kprintf.h"
#include "../cpu/cpu.h"
#include "../kmalloc.h"
#include "../string/string.h"
#include "../sys/syscalls.h"
#include "../cpu/cpu.h"


volatile ProcessControlBlock * gp_process_queue = NULL;
volatile ProcessControlBlock * gp_current_process = NULL;
volatile u64 g_procs = 0;

volatile u64 id_counter = 0;
extern void switch_to_process(void * new_stack, PageTable * cr3);
extern void switch_to_user_proc( void * instruction_ptr, void * stack);
extern void load_pagedir();
extern u64  g_ticks;


CpuData * cpu_data;

void _kill(void){
    
#ifdef SCHEDULER_DEBUG
    kprintf("[SCHEDULER]    Killing process with PID %d\n", gp_current_process->pid);
#endif
    // scheduler takes care of killing zombies
    gp_current_process->state = ZOMBIE;
}

void task_a(){ 
    for(;;)
        kprintf("Running task A...\n");
    //u64 syscall_return = 0;
    //asm volatile(
    //    "pushq %%r11\n"
    //    "pushq %%rcx\n"
    //    "syscall\n"
    //    "popq %%rcx \n"
    //    "popq %%r11 \n"
    //    : "=a"(syscall_return)
    //    : "a"(0)
    //    : "memory");
}

void task_b(){ 
    for(;;){
        kprintf("Running task B...\n");
    }
}

void task_c(){ 
    for(;;)
        kprintf("Running task C...\n");

}
void cleaner(){ 
    for(;;)
        kprintf("Running cleaner ...\n");

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



void kill_zombies(){
    // no procs
    if(g_procs == 0) return;

    ProcessControlBlock * current_pcb = gp_process_queue;
    while(current_pcb->next != NULL){
        ProcessControlBlock * next_pcb = current_pcb->next;
        if(next_pcb->state == ZOMBIE){
            // first relink
            
            if(next_pcb->next != NULL)
                current_pcb->next = next_pcb->next;
            else current_pcb->next = NULL;

#ifdef SCHEDULER_DEBUG
            kprintf("[SCHEDULER]    Proc with PID: %d was killed.\n");
            cli();
            for(;;);
#endif
            // now free memory
            pmm_free_block((u64)next_pcb->cr3);
            pmm_free_block((u64)next_pcb->p_stack);

            --g_procs;
        }

        current_pcb = current_pcb->next;
    }


}

void schedule(Registers * regs){

    CpuData * cpu_data = get_cpu_struct(0);
    cpu_data->saved_proc_stack = regs->rsp;

#ifdef SCHEDULER_DEBUG
    dump_regs(regs);
    kprintf("[SCHEDULER]    %d Global Processes\n", g_procs);
#endif 

    kill_zombies();

    // not enough procs or not time to switch yet
    if(g_procs == 0 || g_procs == 1 || g_ticks % SMP_TIMESLICE != 0) 
        return;

    // save current proc 
    gp_current_process->p_stack = (void*)regs->rsp;

    u64 * stack = gp_current_process->p_stack;
    *--stack = regs->ss; // ss
    *--stack = (u64)gp_current_process->p_stack; // rsp
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

    
    cpu_data = get_cpu_struct(0);
    cpu_data->saved_proc_stack = (u64)stack;

    if(gp_current_process->next == NULL) {
        gp_current_process = gp_process_queue; // go to head

#ifdef SCHEDULER_DEBUG
        kprintf("[SCHEDULER] Switching to head:\n");
        dump_regs((Registers*)gp_current_process->p_stack);
#endif

    } else if (gp_current_process->next != NULL){
        // move to next proc
        gp_current_process = gp_current_process->next;
#ifdef SCHEDULER_DEBUG
        kprintf("[SCHEDULER] Switching to next: \n");
        dump_regs((Registers*)gp_current_process->p_stack);
#endif

    }

    // finally, switch
    switch_to_process(gp_current_process->p_stack, gp_current_process->cr3);

}


ProcessControlBlock * create_kernel_process(void (*entry)(void)){
    // TODO: need allocator
    ProcessControlBlock * pcb =  (ProcessControlBlock*)kmalloc(sizeof(ProcessControlBlock));
        //kmalloc(sizeof(ProcessControlBlock));

    memset(pcb, 0, sizeof(ProcessControlBlock));

    pcb->p_stack = pmm_alloc_block()+0x1000;
    pcb->pid = ++id_counter;

	u64 * stack = (u64 *)(pcb->p_stack);
    PageTable * pml4 = vmm_create_user_proc_pml4(stack);

    // user proc
	*--stack = 0x10; // ss
	*--stack = (u64)pcb->p_stack; // rsp
	*--stack = 0x202 ; // rflags
	*--stack = 0x8; // cs
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

    pcb->state = READY;
    pcb->p_stack = stack;
    pcb->cr3 = pml4; 
    pcb->next = NULL;

    return pcb; 

}

ProcessControlBlock * create_user_process(void (*entry)(void)){
    // TODO: need allocator
    ProcessControlBlock * pcb = (ProcessControlBlock*)kmalloc(sizeof(ProcessControlBlock));
        //kmalloc(sizeof(ProcessControlBlock));

    memset(pcb, 0, sizeof(ProcessControlBlock));

    pcb->p_stack = pmm_alloc_block();
    pcb->pid = ++id_counter;

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

    pcb->state = READY;
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

    register_process(create_kernel_process(cleaner));
    register_process(create_user_process(task_a));
    register_process(create_user_process(task_b));
    gp_current_process = gp_process_queue;

    switch_to_process(gp_current_process->p_stack, gp_current_process->cr3);
    asm volatile ("sti");
}
