#include "../config.h"
#include "proc.h"
#include "../fs/vfs.h"
#include "../memory/pmm.h"
#include "../kprintf.h"
#include "../kmalloc.h"
#include "../string/string.h"
#include "../drivers//video.h"

ProcessControlBlock * gp_process_queue = NULL;
ProcessControlBlock * gp_current_process = NULL;

extern void switch_to_process(void * new_stack, PageTable * cr3);

extern void load_pagedir();
extern u64  g_ticks;

u64 g_procs = 0;

PageTable * kernel_cr3;

void unmap_fd_to_proc(ProcessControlBlock * proc, int file){
    proc->fd_table[file] = NULL;
    --proc->fd_length;
}

void map_fd_to_proc(ProcessControlBlock * proc, struct file * file_desc){
    for(u8 fd_idx = 0; fd_idx < MAX_PROC_FDS; ++fd_idx){
        if(proc->fd_table[fd_idx] == NULL)
            proc->fd_table[fd_idx] = file_desc;
    }
    return;
}

void kill_proc(ProcessControlBlock * proc){

    // Need kernel cr3 for kernel data structures...
    load_pagedir(kernel_cr3);

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
    // test syscall
    asm volatile("mov $0, %rsi\n\t\
        syscall");
}

void task_b(){ 

    /* exit syscall */
    asm volatile("mov $0, %rdi\n\t\
                syscall");
    
    // this should never happen
    for(;;) kprintf("Running task B...\n");

}

void task_c(){ 
    for(;;)
        kprintf("Running task C...\n");

}
void idle_task(){ 
    for(;;) kprintf("Idling...\n");
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



// Creates a process with given entrypoint
// this requires allocating a stack, setting the stack
// and creating page tables
ProcessControlBlock * create_process(void (*entry)(void)){
    ProcessControlBlock * pcb = kmalloc(sizeof(ProcessControlBlock));

    memset(pcb, 0, sizeof(ProcessControlBlock));

    pcb->p_stack = pmm_alloc_block()+PAGE_SIZE;

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


	*--stack = 0; // rbp

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
    return;
}

void multitasking_init(){
    // save kernel page tables

    kernel_cr3 = vmm_get_current_cr3();
    //register_process(create_process(idle_task));
    //register_process(create_process(refresh_screen_proc));
    gp_current_process = gp_process_queue;

    kprintf("Switching to process with 0x%llx cr3\n", gp_current_process->cr3);
    switch_to_process(gp_current_process->p_stack, gp_current_process->cr3);
}
