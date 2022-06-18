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

#define PAGING_KERNEL_OFFSET        0xffffffff80000000
#define PAGING_VIRTUAL_OFFSET       0xffff800000000000

extern void load_pagedir(PageTable *);
extern void invalidate_tlb();
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
        memset(addr, 0, PAGE_SIZE);

        kprintf("allocated block at 0x%x\n", addr);

        if(addr == NULL)
            for(;;); // panic here 
        
        table[entry] = (uintptr_t)addr | PAGE_USER | PAGE_WRITE | PAGE_PRESENT;
    }

    return addr;
}


void vmm_map_page(PageTable * pml4, uintptr_t virt, uintptr_t phys, int flags){
    PageIndex indices = vmm_get_page_index(virt);

    kprintf("[VMM]  Got following indices for %llx PML4I: %d PML3I: %d PML2I: %d PML1I: %d\n", virt, indices.pml4i, indices.pml3i,  indices.pml2i, indices.pml1i);
    uintptr_t * pml3 = get_next_table((uintptr_t*)pml4, indices.pml4i);
    uintptr_t * pml2 = get_next_table(pml3, indices.pml3i);
    uintptr_t * pml1 = get_next_table(pml2, indices.pml2i);
    uintptr_t * p_pte = &pml1[indices.pml1i];



    kprintf("[VMM] pml3 0x%x; pml2 0x%x; pml1 0x%x; p_pte : 0x%x;\n", pml3, pml2, pml1, p_pte);
    kprintf("Entry val before 0x%x\n", *p_pte);
    *p_pte = phys | (flags & 0x7);
    kprintf("Phys address is 0x%x\n", phys);
    kprintf("Flags value is %x\n", flags);
    kprintf("Entry val after 0x%x\n", *p_pte);

    return;
}


void * vmm_virt_to_phys(PageTable * cr3, void * virt_addr){
    kprintf("[VMM]  Translating address: 0x%x\n", virt_addr);

    PageIndex index = vmm_get_page_index((u64) virt_addr);
    PageTableEntry pte;
    kprintf("[VMM]  Got following index. PML4I: %d PML3I: %d PML2I: %d PML1I: %d\n", index.pml4i, index.pml3i,  index.pml2i, index.pml1i);

    pte = cr3->entries[index.pml4i];
    PageTable * pdp;
    if(!pte.present){
        kprintf("[VMM]  PML4 Entry NOT PRESENT\n");
        return 0x0;
    } else pdp = (PageTable*)((u64) pte.address << 12);

    pte = pdp->entries[index.pml3i];
    PageTable * pd;
    if(!pte.present){
        kprintf("[VMM]  PML3 Entry NOT PRESENT\n");
        return 0x0;
    } else pd = (PageTable*)((u64) pte.address << 12);

    pte = pd->entries[index.pml2i];
    PageTable * pt;
    if(!pte.present){
        kprintf("[VMM]  PML2 Entry NOT PRESENT\n");
        return 0x0;
    } else pt = (PageTable*)((u64) pte.address << 12);

    pte = pt->entries[index.pml1i];
    if(!pte.present){
        kprintf("[VMM]  PML1 Entry NOT PRESENT\n");
        return 0x0;
    } 

    return (void*)((pte.address << 12) + ((u64)virt_addr & 0xfff));
}


void vmm_map_range(
    PageTable * cr3, 
    void * virt_start, 
    void * phys_start, 
    size_t size,
    int flags )
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
    kprintf("[VMM] Mapping range");
    kprintf("[VMM] virt_start is 0x%x\n", virt_start);
    kprintf("[VMM] phys_start is 0x%x\n", phys_start);
    kprintf("[VMM] size is 0x%x\n", size);
    kprintf("[VMM] virt_end is 0x%x\n", size);
#endif

    for(; vaddr < (virt_start+size); vaddr+=PAGE_SIZE, paddr+=PAGE_SIZE)
        vmm_map_page(cr3, (uintptr_t)vaddr, (uintptr_t)paddr, flags);

    return;
}



PageTable * vmm_create_user_proc_pml4(void * stack_top){
    PageTable * pml4 = pmm_alloc_block();

    memset(pml4, 0x0, PAGE_SIZE);

    int uflags = PAGE_USER | PAGE_WRITE | PAGE_PRESENT;

    /* map framebuffer */
    for(u64 addr = (u64)get_framebuffer_addr(); 
        addr < ((u64)get_framebuffer_addr()+get_framebuffer_size()); addr+=PAGE_SIZE)
        vmm_map_page(pml4, (void*)addr, (void*)addr-PAGING_VIRTUAL_OFFSET, uflags);

    /* map kernel as user accessible for now*/
    int kflags = PAGE_PRESENT | PAGE_WRITE;
    for(u64 addr = (u64)&k_start; addr < (u64)(&k_end)+PAGE_SIZE; addr+=PAGE_SIZE)
        vmm_map_page(pml4, (void*)addr, (void*)addr-PAGING_KERNEL_OFFSET, kflags);

    
    for(int p= 0; p<8; p++) 
        vmm_map_page(pml4, stack_top-(p*PAGE_SIZE), stack_top-(p*PAGE_SIZE), uflags);

    LocalCpuData * lcd = get_cpu_struct(0); 
    /* map kernel stack */
    uintptr_t kstack_top = lcd->syscall_kernel_stack;

    kprintf("Kernel stack top 0x%x\n", kstack_top);
    for(int p = 0; p<8; p++)
        vmm_map_page(pml4, kstack_top-(p*PAGE_SIZE), kstack_top-(p*PAGE_SIZE), kflags);

    return pml4;
}


PageTable * vmm_get_current_cr3(){
    PageTable * current_cr3;
    asm volatile (" mov %%cr3, %0" : "=r"(current_cr3));
    return current_cr3;
}


