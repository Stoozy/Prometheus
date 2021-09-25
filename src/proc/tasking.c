#include "../config.h"
#include "tasking.h"
#include "../memory/pmm.h"
#include "../kprintf.h"
#include "../kmalloc.h"
#include "../string/string.h"



ProcessControlBlock * gp_process_queue;
ProcessControlBlock * gp_current_process;

extern void switch_to_process(void * new_stack, PageTable * cr3);
extern u64  g_ticks;

volatile u64 g_procs;

void task_a(void);
void idle_task(void);

void idle_task(){ 
    for(;;);
}


void dump_regs(void * stack){
    Registers * regs = (Registers*) stack;
    kprintf("[SCHEDULER]    RIP: 0x%x\n", regs->rip);
    kprintf("[SCHEDULER]    RSP: 0x%x\n", regs->rsp);
    kprintf("[SCHEDULER]    RBP: 0x%x\n", regs->rbp);
    kprintf("[SCHEDULER]    RBX: 0x%x\n", regs->rbx);
    kprintf("[SCHEDULER]    RSI: 0x%x\n", regs->rsi);
    kprintf("[SCHEDULER]    RDI: 0x%x\n", regs->rdi);
}

void schedule(Registers * regs){

#ifdef SMP_DEBUG
    dump_regs(regs);
    kprintf("[SCHEDULER]    %d Global Processes\n", g_procs);
#endif 

    if(g_ticks % SMP_TIMESLICE != 0) return; // not time to switch yet

    // save current proc 
    gp_current_process->p_stack = (void*)regs->rsp;

    u64 * stack = gp_current_process->p_stack;
    *--stack = 0x30; // ss
    *--stack = (u64)gp_current_process->p_stack; // rsp
    *--stack = 512 ; // rflags
    *--stack = 0x28; // cs
    *--stack = (u64)regs->rip; // rip

    *--stack = (u64)regs->rbp; // rbp
    *--stack = 0; // rdx
    *--stack = 0; // rsi
    *--stack = 0; // rdi

    gp_current_process->p_stack = stack;

    // reached tail, go to head
    if(gp_current_process->next == NULL){

        // switch to head
        gp_current_process = gp_process_queue;

#ifdef SMP_DEBUG
        kprintf("[SCHEDULER] Switching to head:\n");
        dump_regs(gp_current_process->p_stack);
#endif
        switch_to_process(gp_current_process->p_stack, gp_current_process->cr3);
    }else{

        // just go to next process 
        gp_current_process = gp_current_process->next;

#ifdef SMP_DEBUG
        kprintf("[SCHEDULER] Switching to next:\n");
        dump_regs(gp_current_process->p_stack);
#endif
        switch_to_process(gp_current_process->p_stack, gp_current_process->cr3);
    }

}

ProcessControlBlock * create_process(void (*entry)(void)){
    ProcessControlBlock * pcb = kmalloc(sizeof(ProcessControlBlock));

    pcb->p_stack = pmm_alloc_block();

	u64 * stack = (u64 *)(pcb->p_stack+0x1000);

	*--stack = 0x30; // ss
	*--stack = (u64)pcb->p_stack; // rsp
	*--stack = 512 ; // rflags
	*--stack = 0x28; // cs
	*--stack = (u64)entry; // rip

	*--stack = (u64)pcb->p_stack ; //ebp
	*--stack = 0; // rdx
	*--stack = 0; // rsi
	*--stack = 0; // rdi

    pcb->p_stack = stack;

    pcb->cr3 = vmm_get_current_cr3();
    pcb->next = NULL;

    ++g_procs;

    return pcb; 
}

void register_process(ProcessControlBlock * new_pcb){
    ProcessControlBlock * current_pcb = gp_process_queue;  

    // first process
    if(current_pcb == NULL){
        gp_process_queue = new_pcb;
        return;
    }

    while(current_pcb->next != NULL)
        current_pcb = current_pcb->next;
    current_pcb->next = new_pcb;

    ++g_procs;
}

void multitasking_init(){
    gp_process_queue = NULL;
    gp_current_process = NULL;

    g_procs = 0;

    asm volatile ("cli");
    register_process(create_process(idle_task));
    gp_current_process->next = gp_process_queue;
    asm volatile ("sti");
}
