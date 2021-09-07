#include "tasking.h"
#include "../memory/pmm.h"
#include "../kprintf.h"
#include "../kmalloc.h"
#include "../string/string.h"

#define MAX_PROCS   256

ProcessControlBlock * gp_process_queue;
ProcessControlBlock * gp_current_process;

ProcessControlBlock p_table[MAX_PROCS];

extern void switch_to_process(void * new_stack);
extern u64 g_ticks;

volatile u64 g_procs;

void task_a(void);
void idle_task(void);

void idle_task(){ 
    for(;;) kprintf("Idling...[%d procs]\n", g_procs);
}

void task_a(void){ 
    for(;;) kprintf("Running task A...[%d procs]\n", g_procs);
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
    //dump_regs(regs);
    kprintf("[SCHEDULER]    %d Global Processes\n", g_procs);
    if(g_procs != 2 || g_ticks % 20 != 0) return;


    // reached tail, go to head
    if(gp_current_process->next == NULL){
        // save current proc 
        gp_current_process->p_stack = (void*)regs->rsp;

        u64 * stack = gp_current_process->p_stack;
        *--stack = 0x30; // ss
        *--stack = (u64)gp_current_process->p_stack; // rsp
        *--stack = 512 ; // rflags
        *--stack = 0x28; // cs
        *--stack = (u64)regs->rip; // rip

        *--stack = (u64)regs->rbp; //ebp
        *--stack = 0; // rdx
        *--stack = 0; // rsi
        *--stack = 0; // rdi

        gp_current_process->p_stack = stack;

        // switch to head
        dump_regs(gp_current_process->p_stack);

        gp_current_process = gp_process_queue;
        kprintf("[SCHEDULER] Switching to head:\n");
        dump_regs(gp_current_process->p_stack);
        switch_to_process(gp_current_process->p_stack);
    }else{
        // just go to next process 
        //dump_regs(gp_process_queue->next->p_stack-sizeof(Registers));

        // save current proc 
        kprintf("[SCHEDULER] Switching to next:\n");
        gp_current_process->p_stack = (void*)regs->rsp;


        u64 * stack = gp_current_process->p_stack;
        *--stack = 0x30; // ss
        *--stack = (u64)gp_current_process->p_stack; // rsp
        *--stack = 512 ; // rflags
        *--stack = 0x28; // cs
        *--stack = (u64)regs->rip; // rip

        *--stack = (u64)regs->rbp; //ebp
        *--stack = 0; // rdx
        *--stack = 0; // rsi
        *--stack = 0; // rdi

        gp_current_process->p_stack = stack;


        gp_current_process = gp_current_process->next;
        //dump_regs(gp_current_process->p_stack);
        switch_to_process(gp_current_process->p_stack);
    }
}

ProcessControlBlock * create_process(void (*entry)(void)){
    ProcessControlBlock * pcb = kmalloc(sizeof(ProcessControlBlock));

    pcb->p_stack = pmm_alloc_block();

    //Registers * regs = (Registers*) pcb->p_stack;
    //memset(regs, 0, sizeof(Registers));

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
        gp_current_process = gp_process_queue;
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

    gp_process_queue = create_process(idle_task);
    gp_process_queue->next = create_process(task_a);

    dump_regs(gp_process_queue->p_stack);
    dump_regs(gp_process_queue->next->p_stack);

    gp_current_process->next = gp_process_queue;

    //for(;;);
    asm volatile ("sti");
    //ProcessControlBlock * new_pcb = create_process(idle_task);
    //register_process(new_pcb);

    //new_pcb = create_process(task_a);
    //register_process(new_pcb);

}
