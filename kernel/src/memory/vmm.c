#include "../kprintf.h"
#include "../typedefs.h"
#include <stdbool.h>
#include "../string/string.h"

#include "../stivale.h"
#include "vmm.h"
#include "pmm.h"

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

i32 vmm_map_user(PageTable * pml4, void * virt_addr, void* phys_addr){

    PageIndex indexer = vmm_get_page_index((u64)virt_addr);
    PageTableEntry PTE;

    PTE = pml4->entries[indexer.pml4i];
    PageTable* PDP;
    if (!PTE.present){
        PDP = (PageTable*)pmm_alloc_block();
        memset(PDP, 0, 0x1000);
        PTE.address = (u64)PDP >> 12;
        PTE.present = 1;
        PTE.user = 1;
        PTE.rw = 1;
        pml4->entries[indexer.pml4i] = PTE;
    }
    else PDP = (PageTable*)((u64)PTE.address << 12);


    PTE = PDP->entries[indexer.pml3i];
    PageTable* PD;
    if (!PTE.present){
        PD = (PageTable*)pmm_alloc_block();
        memset(PD, 0, 0x1000);
        PTE.address = (u64)PD >> 12;
        PTE.present = 1;
        PTE.user = 1;
        PTE.rw = 1;
        PDP->entries[indexer.pml3i] = PTE;
    }
    else PD = (PageTable*)((u64)PTE.address << 12);

    PTE = PD->entries[indexer.pml2i];
    PageTable* PT;
    if (!PTE.present){
        PT = (PageTable*)pmm_alloc_block();
        memset(PT, 0, 0x1000);
        PTE.address = (u64)PT >> 12;
        PTE.present = 1;
        PTE.user = 1;
        PTE.rw = 1;
        PD->entries[indexer.pml2i] = PTE;
    }
    else PT = (PageTable*)((u64)PTE.address << 12);

    PTE = PT->entries[indexer.pml1i];
    PTE.address = (u64)phys_addr >> 12;
    PTE.present = 1;
	PTE.user = 1;
    PTE.rw = 1;
    PT->entries[indexer.pml1i] = PTE;


    //invalidate_tlb();
    return SUCCESS;
} /* vmm_map */

i32 vmm_map(PageTable * pml4, void * virt_addr, void* phys_addr){

    PageIndex indexer = vmm_get_page_index((u64)virt_addr);
    PageTableEntry PTE;

    PTE = pml4->entries[indexer.pml4i];
    PageTable* PDP;
    if (!PTE.present){
        PDP = (PageTable*)pmm_alloc_block();
        memset(PDP, 0, 0x1000);
        PTE.address = (u64)PDP >> 12;
        PTE.present = 1;
        PTE.rw = 1;
        pml4->entries[indexer.pml4i] = PTE;
    }
    else PDP = (PageTable*)((u64)PTE.address << 12);


    PTE = PDP->entries[indexer.pml3i];
    PageTable* PD;
    if (!PTE.present){
        PD = (PageTable*)pmm_alloc_block();
        memset(PD, 0, 0x1000);
        PTE.address = (u64)PD >> 12;
        PTE.present = 1;
        PTE.rw = 1;
        PDP->entries[indexer.pml3i] = PTE;
    }
    else PD = (PageTable*)((u64)PTE.address << 12);

    PTE = PD->entries[indexer.pml2i];
    PageTable* PT;
    if (!PTE.present){
        PT = (PageTable*)pmm_alloc_block();
        memset(PT, 0, 0x1000);
        PTE.address = (u64)PT >> 12;
        PTE.present = 1;
        PTE.rw = 1;
        PD->entries[indexer.pml2i] = PTE;
    }
    else PT = (PageTable*)((u64)PTE.address << 12);

    PTE = PT->entries[indexer.pml1i];
    PTE.address = (u64)phys_addr >> 12;
    PTE.present = 1;
    PTE.rw = 1;
    PT->entries[indexer.pml1i] = PTE;


    //invalidate_tlb();
    return SUCCESS;
} /* vmm_map */


PageTable * vmm_create_user_proc_pml4(){
    PageTable * pml4 = pmm_alloc_block();

    memset(pml4, 0x0, PAGE_SIZE);

    /* map kernel */
    for(u64 addr = (u64)&k_start; addr < (u64)(&k_end)+PAGE_SIZE; addr+=PAGE_SIZE){
        vmm_map(pml4, (void*)addr, (void*)addr-PAGING_KERNEL_OFFSET);
    }

    for(u64 addr = 0; addr <  1024 * 4096; addr+=PAGE_SIZE){
        vmm_map(pml4, (void*)addr, (void*)addr);
        vmm_map(pml4, (void*)addr, (void*)addr-PAGING_VIRTUAL_OFFSET);
    }

    return pml4;
}

PageTable * vmm_get_current_cr3(){
    PageTable * current_cr3;
    asm volatile (" mov %%cr3, %0" : "=r"(current_cr3));
    return current_cr3;
}

