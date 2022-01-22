#include "syscalls.h"
#include "../proc/proc.h"
#include "../fs/vfs.h"
#include <stdint.h>
#include "../kprintf.h"
#include "../cpu/cpu.h"
#include "../kmalloc.h"
#include "../memory/pmm.h"

static inline uint64_t rdmsr(uint64_t msr){
	uint32_t low, high;
	asm volatile (
		"rdmsr"
		: "=a"(low), "=d"(high)
		: "c"(msr)
	);
	return ((uint64_t)high << 32) | low;
}

static inline void wrmsr(uint64_t msr, uint64_t value){
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

int sys_exit(int exit_code){
    _kill(); 
    return exit_code;
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

volatile void * syscalls[] = {
    &sys_exit,
    &sys_close,
    &sys_execve,
    &sys_fork,
};

void syscall_dispatcher(Registers regs){
    dump_regs(&regs);
    for(;;);

    u64 syscall = regs.rsi;

    //u64 return_addr = regs.rip;
    //u64 rcx = 0;

    switch(syscall){
        case 0:
            sys_exit(regs.r8);
            break;
        default:
            break;
    }
}

void sys_init(){
    
    LocalCpuData * lcd = get_cpu_struct(0);

    wrmsr(EFER, rdmsr(EFER) | 1); // enable syscall
    wrmsr(GSBASE, (u64)lcd); //  GSBase
    wrmsr(KGSBASE, (u64)lcd); // KernelGSBase

    extern void syscall_entry();     // syscall_entry.asm
    wrmsr(LSTAR, (u64)&syscall_entry);

}


