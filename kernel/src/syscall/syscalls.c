#include <stdint.h>
#include <sys/mman.h>

#include "syscalls.h"
#include "../config.h"
#include "../typedefs.h"
#include "../proc/proc.h"
#include "../fs/vfs.h"
#include "../kprintf.h"
#include "../cpu/cpu.h"
#include "../kmalloc.h"
#include "../memory/pmm.h"
#include "../memory/vmm.h"

typedef long int off_t;

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

void sys_log_libc(const char * message){
    kprintf(message);
}


void * sys_anon_allocate(size_t size){ 
    kprintf("[SYS]  sys anon allocate was called\n");
    u64 blocks = (size/PAGE_SIZE) +1;
    kprintf("[SYS]  Requesting %llu  blocks\n");
    void * ret = pmm_alloc_blocks(blocks);
    extern ProcessControlBlock * gp_current_process;

    int flags =  PAGE_PRESENT | PAGE_READ_WRITE | PAGE_USER;
    for(u64 page = 0; page < blocks; page++){
        void * caddr = (void*)(ret + (page * PAGE_SIZE));
        vmm_map(gp_current_process->cr3, caddr, caddr, flags);
    }
    return ret;
}

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

int sys_read(int file, char *ptr, size_t len){
    extern ProcessControlBlock * gp_current_process;
    FILE * f = gp_current_process->fd_table[file];
    int bytes_read = vfs_read(f, (u64)len, (u8*)ptr);

    return bytes_read;
}

int sys_write(int file, char *ptr, int len){
    //if(file == 1){
        // trying to print out to stdout
        // output to serial for now...
   
        // FIXME: should probably start
        // at the file position instead
        
#ifdef SYSCALL_DEBUG
    kprintf("Called SYS_WRITE File: %d Buffer addr: 0x%x Length: %d\n", file, ptr, len);
#endif 
        for(int i=0; i<len; ++i)
            kprintf("%c", ptr[i]);
        kprintf("\n");
        return len;
    //}


    //extern ProcessControlBlock * gp_current_process;
    //FILE * f = gp_current_process->fd_table[file];
    //int bytes_written = vfs_write(f, (u64)len, (u8*)ptr);

    //return bytes_written;
}

/* snatched from mlibc/vm-flags.h */
#define PROT_NONE  0x00
#define PROT_READ  0x01
#define PROT_WRITE 0x02
#define PROT_EXEC  0x04

#define MAP_PRIVATE   0x01
#define MAP_SHARED    0x02
#define MAP_FIXED     0x04
#define MAP_ANON      0x08
#define MAP_ANONYMOUS 0x08


void * sys_vm_map(
    void * addr, 
    size_t size, 
    int prot, 
    int flags,
    int fd, 
    off_t offset )
{
    kprintf("sys_vm_map was called\n");
    /* the calling proc */
    extern ProcessControlBlock * gp_current_process;

    size_t rounded_size;
    rounded_size =  size < PAGE_SIZE ? PAGE_SIZE : ((size+PAGE_SIZE)/PAGE_SIZE) * PAGE_SIZE;



    if( flags & MAP_ANON){

        kprintf("[MMAP] Mapping anon and fixed on cr3 0x%x\n", gp_current_process->cr3);

        if((uint64_t)addr == 0){
            addr = (void*)gp_current_process->mmap_base;            
            gp_current_process->mmap_base += rounded_size;
            kprintf("[MMAP] Address was NULL: new addr 0x%x\n", addr);
        }
        int page_flags =  PAGE_PRESENT | PAGE_USER;
        if(prot & PROT_WRITE)
            page_flags |= PAGE_READ_WRITE;
    
        void * paddr = pmm_alloc_blocks(rounded_size/PAGE_SIZE);
        if(paddr == NULL){
            kprintf("Not enough memory!\n");
            return -1;
        }
        for(void* vaddr = addr; (u64)vaddr < ((u64)addr+rounded_size); vaddr += PAGE_SIZE, paddr += PAGE_SIZE){
            vmm_map(gp_current_process->cr3, (void*)vaddr, paddr, page_flags);
        }
        extern void invalidate_tlb();
        invalidate_tlb();
    }

    return addr;
}

int sys_execve(char *name, char **argv, char **env){

    return -1;
}


int sys_fork(){
    return -1; 
}

void syscall_dispatcher(Registers regs){
#ifdef SYSCALL_DEBUG
    dump_regs(&regs);
#endif

    u64 syscall = regs.rsi;
    u64 ret = 0;

    switch(syscall){
        case SYS_EXIT:{
            register int status asm("r9");
            sys_exit();
            break;
        }
        case SYS_OPEN:{
            register const char * fp asm("r8");
            register int flags asm("r9");
            register int * fd asm("r10");
            register int ret asm("r15") = sys_open(fp, flags);
            
            break;
        }
        case SYS_CLOSE:{
            break;
        }
        case SYS_READ:{
            register int fd asm("r8");
            register char * buf asm("r9");
            register size_t count asm("r10");
            register int ret asm("r15") = sys_read(fd, buf, count);
            break;

        }
        case SYS_WRITE:{
            register int file asm("r8");
            register char * ptr asm("r9");
            register int len asm("r10");
            register int ret asm("r15") = sys_write(file, ptr, len);
            break;
        }
        case SYS_LOG_LIBC: {
            register const char * msg asm("r8");  
            sys_log_libc(msg);
            break;
        }
        case SYS_VM_MAP:{
            register void* addr asm("r8");
            register size_t size asm("r9");
            register int prot asm("r10");
            register int flags asm("r10");
            register int fd asm("r12");
            register off_t off asm("r13");
            register void * ret asm ("r15") = sys_vm_map(addr, size, prot, flags, fd, off);
            break;
        }
        case SYS_ANON_ALLOC:{
            register size_t size asm("r8");
            register void * ret asm ("r15") = sys_anon_allocate(size);
        }
        default:{
            break;
        }
    }

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


