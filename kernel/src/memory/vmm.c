#include "../kprintf.h"
#include "../typedefs.h"
#include <stdbool.h>
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

#define SUCCESS     1

extern void load_pagedir(PageTable *);
extern void invalidate_tlb();
extern u64 k_start;
extern u64 k_end;


PageIndex vmm_get_page_index(u64 vaddr){
    PageIndex ret;

    ret.pml1i = (vaddr >> 12) & 0x1FF;
    ret.pml2i = (vaddr >> 21) & 0x1FF;
    ret.pml3i = (vaddr >> 30) & 0x1FF;
    ret.pml4i = (vaddr >> 39) & 0x1FF;

    return ret;

} /* vmm_get_index */

void * vmm_virt_to_phys(PageTable * cr3, void * virt_addr){
    kprintf("[VMM]  Translating address: 0x%x\n", virt_addr);

    PageIndex index = vmm_get_page_index((u64) virt_addr);
    PageTableEntry pte;
    kprintf("[VMM]  Got following index. PML4I: %d PML3I: %d PML2I: %d PML1I: %d\n", index.pml4i, index.pml3i, 
                    index.pml2i, index.pml1i);

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

i32 vmm_map(PageTable * pml4, void * virt_addr, void* phys_addr, int flags){

#ifdef VMM_DEBUG
    kprintf("[VMM]  Mapping 0x%x virt to 0x%x phys on CR3: 0x%x\n", virt_addr, phys_addr, pml4);
#endif

    PageIndex indexer = vmm_get_page_index((u64)virt_addr);
    PageTableEntry PTE;

    PTE = pml4->entries[indexer.pml4i];
    PageTable* PDP;
    if (!PTE.present){
        PDP = (PageTable*)pmm_alloc_block();
        memset(PDP, 0, PAGE_SIZE);
        PTE.address = (u64)PDP >> 12;
        PTE.present = flags & PAGE_PRESENT;
        PTE.rw = flags & PAGE_READ_WRITE;
        PTE.user = flags & PAGE_USER;
        pml4->entries[indexer.pml4i] = PTE;
    }
    else PDP = (PageTable*)((u64)PTE.address << 12);


    PTE = PDP->entries[indexer.pml3i];
    PageTable* PD;
    if (!PTE.present){
        PD = (PageTable*)pmm_alloc_block();
        memset(PD, 0, PAGE_SIZE);
        PTE.address = (u64)PD >> 12;
        PTE.present = flags & PAGE_PRESENT;
        PTE.rw = flags  & PAGE_READ_WRITE;
        PTE.user = flags & PAGE_USER;
        PDP->entries[indexer.pml3i] = PTE;
    }
    else PD = (PageTable*)((u64)PTE.address << 12);

    PTE = PD->entries[indexer.pml2i];
    PageTable* PT;
    if (!PTE.present){
        PT = (PageTable*)pmm_alloc_block();
        memset(PT, 0, PAGE_SIZE);
        PTE.address = (u64)PT >> 12;
        PTE.present = flags & PAGE_PRESENT;
        PTE.rw = flags & PAGE_READ_WRITE;
        PTE.user = flags & PAGE_USER;
        PD->entries[indexer.pml2i] = PTE;
    }
    else PT = (PageTable*)((u64)PTE.address << 12);

    PTE = PT->entries[indexer.pml1i];
    PTE.address = (u64)phys_addr >> 12;
    PTE.present = flags & PAGE_PRESENT;
    PTE.rw = flags & PAGE_READ_WRITE;
    PTE.user = flags & PAGE_USER;
    PT->entries[indexer.pml1i] = PTE;


    return SUCCESS;
} /* vmm_map */

void vmm_map_range(PageTable * cr3, MemRange range, int flags){

    // page align all addresses
    uintptr_t vc   = (u64)range.vaddr & ~(0xfff);
    uintptr_t pc =  (u64)range.paddr & ~(0xfff);

    uintptr_t end  = (u64)range.vaddr + range.length;
    // page align last page
    end &= ~(0xfff);
    
    while(vc != end){
        vmm_map(cr3, (void*)vc, (void*)pc, flags);
        vc += PAGE_SIZE; pc += PAGE_SIZE;
    }
}

PageTable * vmm_create_kernel_proc_pml4(void * stack_top){
    PageTable * pml4 = pmm_alloc_block();

    memset(pml4, 0x0, PAGE_SIZE);

    int kflags =  PAGE_READ_WRITE | PAGE_PRESENT;

    /* map kernel only */
    for(u64 addr = (u64)&k_start; addr < (u64)(&k_end)+PAGE_SIZE; addr+=PAGE_SIZE)
        vmm_map(pml4, (void*)addr, (void*)addr-PAGING_KERNEL_OFFSET, kflags);

    //void * stack = stack_top-0x1000;
    //vmm_map(pml4, stack, stack, kflags);

    return pml4;
}

PageTable * vmm_create_user_proc_pml4(void * stack_top){
    PageTable * pml4 = pmm_alloc_block();

    memset(pml4, 0x0, PAGE_SIZE);

    int uflags = PAGE_READ_WRITE | PAGE_USER | PAGE_PRESENT;


    /* map framebuffer */
    for(u64 addr = (u64)get_framebuffer_addr(); 
        addr < ((u64)get_framebuffer_addr()+get_framebuffer_size()); addr+=PAGE_SIZE)
        vmm_map(pml4, (void*)addr, (void*)addr-PAGING_VIRTUAL_OFFSET, uflags);

    /* map kernel as user accessible for now*/
    int kflags = PAGE_PRESENT | PAGE_READ_WRITE;
    for(u64 addr = (u64)&k_start; addr < (u64)(&k_end)+PAGE_SIZE; addr+=PAGE_SIZE)
        vmm_map(pml4, (void*)addr, (void*)addr-PAGING_KERNEL_OFFSET, kflags);

    
    for(int p= 0; p<4; p++) 
        vmm_map(pml4, stack_top-(p*PAGE_SIZE), stack_top-(p*PAGE_SIZE), uflags);

    LocalCpuData * lcd = get_cpu_struct(0); 
    /* map kernel stack */
    uintptr_t kstack_top = lcd->syscall_kernel_stack;

    kprintf("Kernel stack top 0x%x\n", kstack_top);
    for(int p = 0; p<4; p++)
        vmm_map(pml4, kstack_top-(p*PAGE_SIZE), kstack_top-(p*PAGE_SIZE), kflags);

    return pml4;
}


PageTable * vmm_get_current_cr3(){
    PageTable * current_cr3;
    asm volatile (" mov %%cr3, %0" : "=r"(current_cr3));
    return current_cr3;
}


