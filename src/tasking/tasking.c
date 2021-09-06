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
    kprintf("[SCHEDULER]    %d Global Processes\n", g_procs);
    if(gp_process_queue == NULL || g_ticks % 20 != 0) return;

    ProcessControlBlock * current_pcb = gp_process_queue;
    // second task doesn't exist, 
    // no need to switch
    // just return because there's
    // only one process
    if(gp_process_queue->next == NULL) return;

    // reached tail, go to head
    if(gp_current_process->next == NULL){
        dump_regs(gp_process_queue->p_stack);
        // save current proc 
        gp_current_process->p_stack = regs->rsp;

        gp_current_process = gp_process_queue;
        switch_to_process(gp_process_queue->p_stack);
        return;
    }

    // just go to next process 
    dump_regs(gp_process_queue->next->p_stack);

    // save current proc 
    gp_current_process->p_stack = regs->rsp;
    gp_current_process = gp_current_process->next;
    switch_to_process(gp_current_process->p_stack);

}

ProcessControlBlock * create_process(void (*entry)(void)){
    ProcessControlBlock * pcb = kmalloc(sizeof(ProcessControlBlock));

    pcb->p_stack = pmm_alloc_block();

    //Registers * regs = (Registers*) pcb->p_stack;
    //memset(regs, 0, sizeof(Registers));

	u64 * stack = (u64 *)(pcb->p_stack+0x1000);
	*--stack = 0x30; // ss
	*--stack = stack; // rsp
	*--stack = 0x00000202 | 1 << 9; // rflags
	*--stack = 0x28; // cs
	*--stack = (u64)entry; // eip
	//*--stack = 0; // eax
	//*--stack = 0; // ebx
	//*--stack = 0; // ecx

	*--stack = (u64)pcb->p_stack ; //ebp
	*--stack = 0; // edx
	*--stack = 0; // esi
	*--stack = 0; // edi
	//*--stack = 0x30; // ds
	//*--stack = 0x30; // fs
	//*--stack = 0x30; // es
	//*--stack = 0x30; // gs
    pcb->p_stack = stack;
    //regs->rip = (u64)entry;
    //regs->rsp = (u64)pcb->p_stack+sizeof(Registers);
    //regs->rflags = 0x202;
    //regs->cs = 0x28;

    pcb->cr3 = vmm_get_current_cr3();
    pcb->next = NULL;

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

    asm volatile("cli");
    g_procs = 0;

    gp_process_queue = create_process(idle_task);
    ++g_procs;

    gp_current_process = gp_process_queue;
    gp_current_process->next = create_process(task_a);
    ++g_procs;

    //ProcessControlBlock * new_pcb = create_process(idle_task);
    //register_process(new_pcb);

    //new_pcb = create_process(task_a);
    //register_process(new_pcb);

    asm volatile("sti");
}
