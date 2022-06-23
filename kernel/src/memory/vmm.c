#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../kprintf.h"
#include "../typedefs.h"
#include "../string/string.h"
#include "../kmalloc.h"
#include "../stivale2.h"
#include "vmm.h"
#include "pmm.h"
#include "../drivers/video.h"
#include "../config.h"
#include "../cpu/cpu.h"

extern void load_pagedir(PageTable *);

extern u64 k_start;
extern u64 k_end;


static PageIndex vmm_get_page_index(uintptr_t vaddr){
    PageIndex ret;

    ret.pml4i = (vaddr & ((uintptr_t) 0x1ff << 39)) >> 39;
    ret.pml3i = (vaddr & ((uintptr_t) 0x1ff << 30)) >> 30;
    ret.pml2i = (vaddr & ((uintptr_t) 0x1ff << 21)) >> 21;
    ret.pml1i = (vaddr & ((uintptr_t) 0x1ff << 12)) >> 12;

    return ret;
}


static uintptr_t * get_next_table(uintptr_t * table, u64 entry){
    void* addr;

    if(table[entry] & PAGE_PRESENT){
        addr = (void*)(table[entry] & ~((uintptr_t) 0xfff));
    }else{
        addr = pmm_alloc_block();
        memset(PAGING_VIRTUAL_OFFSET + addr, 0, PAGE_SIZE);

        if(addr == NULL)
            for(;;); // panic here 
        
        table[entry] = (uintptr_t)addr | PAGE_USER | PAGE_WRITE | PAGE_PRESENT;
    }

    return PAGING_VIRTUAL_OFFSET + addr;
}




void vmm_map_page(PageTable * pml4, uintptr_t virt, uintptr_t phys, int flags){
    PageIndex indices = vmm_get_page_index(virt);

    uintptr_t * pml3 = get_next_table((uintptr_t*)pml4, indices.pml4i);
    uintptr_t * pml2 = get_next_table(pml3, indices.pml3i);
    uintptr_t * pml1 = get_next_table(pml2, indices.pml2i);
    uintptr_t * p_pte = &pml1[indices.pml1i];

    *p_pte = phys | (flags & 0x7);

    return;
}



void vmm_map_range(
    PageTable * cr3, 
    void * virt_start, 
    void * phys_start, 
    size_t size,
    int flags)
{

    if( size % PAGE_SIZE != 0 || 
        (u64)virt_start % PAGE_SIZE != 0 ||
        (u64)phys_start % PAGE_SIZE != 0){
        /* not page aligned */
        kprintf("Range not page aligned");
        for(;;);
    }
    void * vaddr = virt_start;
    void * paddr = phys_start;

    void * virt_end = virt_start + size;

#ifdef VMM_DEBUG 
    kprintf("[VMM] Mapping range on 0x%x\n", cr3);
    kprintf("[VMM] virt_start is 0x%x\n", virt_start);
    kprintf("[VMM] phys_start is 0x%x\n", phys_start);
    kprintf("[VMM] size is 0x%x\n", size);
    kprintf("[VMM] virt_end is 0x%x\n", virt_end);
#endif

    for(; vaddr < (virt_start+size); vaddr+=PAGE_SIZE, paddr+=PAGE_SIZE)
        vmm_map_page(cr3, (uintptr_t)vaddr, (uintptr_t)paddr, flags);

    return;
}


PageTable * vmm_create_user_proc_pml4(void * stack_top){
    PageTable * pml4 = (PageTable*)pmm_alloc_block();
    memset(pml4, 0x0, PAGE_SIZE);

    PageTable * kcr3 = vmm_get_current_cr3();

    for(int i=256; i<512; i++){
        pml4->entries[i] = kcr3->entries[i];
    }

    int uflags = PAGE_USER | PAGE_WRITE | PAGE_PRESENT;

    /* map framebuffer */

    //void* fb_start = get_framebuffer_addr();
    //void* fb_end = fb_start + get_framebuffer_size();

    //size_t fb_size = ((get_framebuffer_size()) / PAGE_SIZE) * PAGE_SIZE;
    //kprintf("[VMM]  Mapping fb for userspace\n");
    //vmm_map_range(pml4, fb_start, fb_start-PAGING_VIRTUAL_OFFSET, fb_size, uflags);

    /* kernel mapping */
    int kflags = PAGE_PRESENT | PAGE_WRITE;
    //extern u64 k_start, k_end, k_size;

    //kprintf("[VMM]  Mapping kernel \n");
    //vmm_map_range(pml4, (void*)k_start , (void*)k_start-PAGING_KERNEL_OFFSET, k_size, kflags);
    
    /* mapping stacks */
    size_t stack_size = 8*PAGE_SIZE;
    void * stack_base = stack_top-(stack_size);
    kprintf("[VMM]  Mapping userspace stack \n");
    vmm_map_range(pml4, stack_base, stack_base, stack_size, uflags);


    //LocalCpuData * lcd = get_cpu_struct(0); 
    /* map kernel stack */
    //void* kstack_base = ((void*)lcd->syscall_kernel_stack) - stack_size;
    //kprintf("[VMM]  Mapping kernel stack 0x%x\n", kstack_base);
    //vmm_map_range(pml4, kstack_base, kstack_base, stack_size, kflags);

    return pml4;
}

void vmm_switch_page_directory(PageTable * cr3){
    load_pagedir(cr3);
    return;
}

PageTable * vmm_get_current_cr3(){
    PageTable * current_cr3;
    asm volatile (" mov %%cr3, %0" : "=r"(current_cr3));
    return current_cr3;
}


