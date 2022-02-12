#include <stdint.h>

#include "syscalls.h"
#include "../typedefs.h"
#include "../proc/proc.h"
#include "../fs/vfs.h"
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

int sys_exit(){
    kill_current_proc();
    return 0;
}

int sys_open(const char *name, int flags, ...){

    FILE * f =  vfs_open(name, flags);

    extern ProcessControlBlock * gp_current_process;
    // error on overflow
    if(gp_current_process->fd_length > MAX_PROC_FDS)
        return -1;

    map_fd_to_proc(gp_current_process, f);
    return gp_current_process->fd_length;
}

int sys_close(int file){
    extern ProcessControlBlock * gp_current_process;
    unmap_fd_to_proc(gp_current_process, file);
    return 0;
}

int sys_read(int file, char *ptr, int len){
    extern ProcessControlBlock * gp_current_process;
    FILE * f = gp_current_process->fd_table[file];
    int bytes_read = vfs_read(f, (u64)len, (u8*)ptr);

    return bytes_read;
}

int sys_write(int file, char *ptr, int len){
    cli();
    for(;;);
    //if(file == 1){
        // trying to print out to stdout
        // output to serial for now...
   
        // FIXME: should probably start
        // at the file position instead
        for(u64 i=0;i<len; i++)
            kprintf("%c", ptr[i]);
        return len;
    //}


    //extern ProcessControlBlock * gp_current_process;
    //FILE * f = gp_current_process->fd_table[file];
    //int bytes_written = vfs_write(f, (u64)len, (u8*)ptr);

    //return bytes_written;
}

int sys_execve(char *name, char **argv, char **env){

    return -1;
}


int sys_fork(){
    return -1; 
}

void syscall_dispatcher(Registers regs){
    for(;;);
    dump_regs(&regs);

    u64 syscall = regs.rsi;
    u64 ret = 0;

    switch(syscall){
        case 0:
            ret = (u64)sys_exit();
            break;
        case 1:
            ret = (u64)sys_open((const char *)regs.r8, (int)regs.r9);
            break;
        case 2:
            ret = (u64)sys_close((int)regs.r8);
            break;
        case 3:
            ret = (u64)sys_read((int)regs.r8, (char*)regs.r9, (int)regs.r10);
            break;
        case 4:
            ret = (u64)sys_write((int)regs.r8, (char*)regs.r9, (int)regs.r10);
            break;
        case 5:
            ret = (u64)sys_execve((char*)regs.r8, (char **)regs.r9, (char **)regs.r10);
        default:
            break;
    }

    // set return value
    regs.r15 = (u64)ret;
}

void sys_init(){
    cpu_init(0);
    LocalCpuData * lcd = get_cpu_struct(0);

    wrmsr(EFER, rdmsr(EFER) | 1); // enable syscall

    extern void enable_sce();
    enable_sce();

    wrmsr(GSBASE, (u64)lcd); //  GSBase
    wrmsr(KGSBASE, (u64)lcd); // KernelGSBase
    wrmsr(SFMASK, (u64)0); // KernelGSBase
    
    extern void syscall_entry();     // syscall_entry.asm
    wrmsr(LSTAR, (u64)&syscall_entry);
}


