#include "syscalls.h"
#include "../proc/proc.h"
#include "../fs/vfs.h"
#include <stdint.h>
#include "../kprintf.h"

static inline void wrmsr(uint64_t msr, uint64_t value)
{
	uint32_t low = value & 0xFFFFFFFF;
	uint32_t high = value >> 32;
	asm volatile (
		"wrmsr"
		:
		: "c"(msr), "a"(low), "d"(high)
	);
}

extern void set_kernel_entry(void * rip);

char * __env = {0};
/* pointer to array of char * strings that define the current environment variables */
char **environ = &__env; 

int sys_exit(){
    asm volatile ("cli");
    for(;;) kprintf("SYS EXIT WAS CALLED \n");
    _kill(); 
    asm volatile ("sti");
    return 0;
}

int sys_close(int file){
    // TODO 
    return -1;
}

int sys_execve(char *name, char **argv, char **env){
    // TODO 
    int f = vfs_open(name, 0);
    return -1;
}


int sys_fork(){
    return -1; 
}

void * syscalls[] = {
    &sys_exit,
    &sys_close,
    &sys_execve,
    &sys_fork,
};

void syscall_dispatcher(){

    register long long syscall_index __asm__ ("rax");

    void (*func)() = syscalls[syscall_index];
    (*func)();

}

void sys_init(){
    void * syscall_entry = &syscall_dispatcher;
    wrmsr(0xc0000082, (u64)syscall_entry);
}


