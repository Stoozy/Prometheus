#include "cpu/cpu.h"
#include "cpu/idt.h"
#include "libk/util.h"
#include "memory/vmm.h"
#include <config.h>
#include <drivers/video.h>
#include <fs/vfs.h>
#include <libk/kmalloc.h>
#include <libk/kprintf.h>
#include <memory/pmm.h>
#include <proc/elf.h>
#include <proc/proc.h>
#include <stdint.h>
#include <string/string.h>

extern u64 g_ticks;
extern void switch_to_process(void *new_stack, PageTable *cr3);
extern void load_pagedir();

volatile ProcessQueue ready_queue = {0, NULL, NULL};
volatile ProcessQueue wait_queue = {0, NULL, NULL};
volatile ProcessControlBlock *running = NULL;

PageTable *kernel_cr3 = NULL;

uint64_t pid_counter = 200;

void dump_pqueue(ProcessQueue *q) {
    ProcessControlBlock *cnode = q->first;
    for (; cnode; cnode = cnode->next) {
        kprintf("%s [%d] -> ", cnode->name, cnode->pid);
    }
    kprintf("NULL \n");
}

void pqueue_push(ProcessQueue *queue, ProcessControlBlock *data) {
    if (!queue->first) {
        queue->count = 1;
        queue->first = queue->last = data;
        return;
    }

    queue->last->next = data;
    queue->last = queue->last->next;
    queue->count++;
    return;
}

ProcessControlBlock *pqueue_pop(ProcessQueue *queue) {
    if (!queue->first)
        return NULL;

    ProcessControlBlock *tmp = queue->first;

    queue->first = queue->first->next;
    queue->count--;

    return tmp;
}

void pqueue_remove(ProcessQueue *queue, int pid) {
    ProcessControlBlock *v = pqueue_pop(queue);
    while (v && v->pid != pid) {
        v->next = NULL;
        pqueue_push(queue, v);
        v = pqueue_pop(queue);
    }
}

void unmap_fd_from_proc(ProcessControlBlock *proc, int fd) {
    if (fd > MAX_PROC_FDS || fd < 0)
        return;

    proc->fd_table[fd] = NULL;
    --proc->fd_length;
    return;
}

int map_file_to_proc(ProcessControlBlock *proc, struct file *file) {
    for (uint64_t fd = 3; fd < MAX_PROC_FDS; ++fd) {
        if (proc->fd_table[fd] == NULL) {
            proc->fd_table[fd] = file;
            ++proc->fd_length;
            return fd;
        }
    }

    return -1;
}

void kill_proc(ProcessControlBlock *proc, int exit_code) {
    // TODO: cleanup actual process

    disable_irq();
    kprintf("Before removing\n");
    dump_pqueue(&ready_queue);

    pqueue_remove(&ready_queue, proc->pid);

    proc->exit_code = exit_code;
    proc->state = ZOMBIE;

    if (proc->parent)
        proc->parent->childDied = true;

    kprintf("After removing\n");
    dump_pqueue(&ready_queue);

    // if (proc->parent)
    //     pqueue_remove(&wait_queue, proc->parent);

    enable_irq();
    for (;;)
        ;
}

void kill_cur_proc(int exit_code) { kill_proc(running, exit_code); }

void task_a() {
    extern uint8_t kbd_read_from_buffer();
    for (;;) {
        kprintf("Running task A ...\n");
        uint8_t c = kbd_read_from_buffer();
        for (;;)
            kprintf("Task A woke up, got character %c \n", c);
    }
}

void task_b() {
    for (;;)
        kprintf("Running task B...\n");
}
void idle_task() {
    for (;;)
        kprintf("Idling...\n");
}

void dump_proc_vas(ProcessControlBlock *proc) {
    VASRangeNode *cnode = proc->vas;
    kprintf("---------PROCESS VAS---------\n");

    if (proc->vas) {
        kprintf("start: %x; size: %x; flags: %d\n", cnode->virt_start,
                cnode->size, cnode->page_flags);

        while (cnode->next != NULL) {
            kprintf("start: %x; size: %x; flags: %d\n", cnode->virt_start,
                    cnode->size, cnode->page_flags);

            cnode = cnode->next;
        }
    }
}

void proc_add_vas_range(ProcessControlBlock *proc, VASRangeNode *node) {
    if (!proc->vas)
        proc->vas = node;

    VASRangeNode *cnode = proc->vas;

    while (cnode->next != NULL)
        cnode = cnode->next;

    cnode->next = node;
    node->next = NULL;
}

ProcessControlBlock *create_kernel_process(void (*entry)(void), char *name) {

    ProcessControlBlock *pcb = kmalloc(sizeof(ProcessControlBlock));
    memset(pcb, 0, sizeof(ProcessControlBlock));

    memcpy(&pcb->name, name, 256);

    void *stack_ptr = (void *)pmm_alloc_blocks(8) + 8 * PAGE_SIZE;
    memset(&pcb->trapframe, 0, sizeof(Registers));

    pcb->trapframe.ss = 0x10;
    pcb->trapframe.rsp = (uint64_t)stack_ptr;
    pcb->trapframe.rflags = 0x202;
    pcb->trapframe.cs = 0x08;
    pcb->trapframe.rip = (uint64_t)entry;

    pcb->cr3 = vmm_get_current_cr3(); // kernel cr3
    pcb->state = READY;
    pcb->pid = pid_counter++;
    pcb->next = NULL;

    return pcb;
}

ProcessControlBlock *clone_process(ProcessControlBlock *proc, Registers *regs) {

    ProcessControlBlock *clone = kmalloc(sizeof(ProcessControlBlock));
    *clone = *proc;

    // reset vas so that proper phys addrs get put by vmm_copy_vas
    clone->vas = NULL;

    vmm_copy_vas(clone, proc);

    clone->pid = pid_counter++;

    clone->trapframe = *regs;
    clone->trapframe.rax = 0;

    kprintf("Registers are at stack %x\n", regs);
    kprintf("Fork'd process has cr3: %x\n", clone->cr3);

    pqueue_push(&proc->children, clone);

    clone->parent = proc;

    return clone;
}

void register_process(ProcessControlBlock *new_pcb) {
    new_pcb->next = NULL;
    pqueue_push(&ready_queue, new_pcb);
    return;
}

void block_process(ProcessControlBlock *proc, int reason) {
    // scheduler will handle the rest
    // note: the rest isn't done here
    // because the registers have to
    // be saved once the interrupt fires
    proc->state = reason;
    kprintf("Blocking process at %x\n", proc);
    return;
}

void unblock_process(ProcessControlBlock *proc) {
    proc->state = READY;
    proc->next = NULL;
    pqueue_push(&ready_queue, proc);
    kprintf("\n unblocked %s (%d)\n\n", proc->name, proc->pid);
    return;
}

void multitasking_init() {
    // save kernel page tables
    kernel_cr3 = vmm_get_current_cr3();

    memset(&ready_queue, 0, sizeof(ProcessQueue));
    memset(&wait_queue, 0, sizeof(ProcessQueue));

    char *envp[5] = {"SHELL=/usr/bin/bash", "PATH=/usr/bin", "HOME=/",
                     "TERM=vt100", NULL};
    char *argvp[2] = {"/usr/bin/bash", NULL};

    // ProcessControlBlock *nomterm =
    //     create_elf_process("/usr/bin/nomterm", argvp, envp);
    // kprintf("Got process at %x\n", nomterm);
    // register_process(nomterm);

    extern void refresh_screen_proc();
    extern void terminal_main();

    // register_process(create_kernel_process(task_a));
    // register_process(create_kernel_process(idle_task));
    // register_process(create_kernel_process(task_b));

    ProcessControlBlock *bash =
        create_elf_process("/usr/bin/init", argvp, envp);
    register_process(create_kernel_process(terminal_main, "Terminal"));
    register_process(bash);
    register_process(create_kernel_process(refresh_screen_proc, "Screen"));

    running = ready_queue.first;
    kprintf("Ready queue has %d procs\n", ready_queue.count);
    dump_pqueue(&ready_queue);

    dump_regs(&ready_queue.first->trapframe);
    kprintf("switching to %s (pid:%d)\n", running->name, running->pid);
    kprintf("Cr3 is %x\n", running->cr3);
    switch_to_process(&running->trapframe, (void *)running->cr3);
    return;
}
