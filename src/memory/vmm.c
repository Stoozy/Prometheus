#include "../kprintf.h"
#include "../typedefs.h"
#include <stdbool.h>
#include "../string/string.h"

#include "vmm.h"
#include "pmm.h"
#define PAGE_SIZE   4096
#define SUCCESS     1

extern void load_pagedir();
extern void invalidate_tlb();


volatile PageIndex g_page_index;
PageTable * gp_pml4;

PageIndex vmm_get_page_index(u64 vaddr){
    vaddr >>= 12;
    g_page_index.pml1i = vaddr & 0x1ff;

    vaddr >>= 9; 
    g_page_index.pml2i = vaddr & 0x1ff;

    vaddr >>= 9;
    g_page_index.pml3i = vaddr & 0x1ff;

    vaddr >>= 9;
    g_page_index.pml4i = vaddr & 0x1ff;

    return g_page_index;
} /* vmm_get_index */

i32 vmm_map(void * virt_addr, void* phys_addr){

    PageIndex indexer = vmm_get_page_index((u64)virt_addr);
    PageTableEntry PDE;

    PDE = gp_pml4->entries[indexer.pml4i];
    PageTable* PDP;
    if (!PDE.present){
        PDP = (PageTable*)pmm_alloc_block();
        memset(PDP, 0, 0x1000);
        PDE.address = (u64)PDP >> 12;
        PDE.present = true;
        PDE.rw = true;
        gp_pml4->entries[indexer.pml4i] = PDE;
    }
    else
    {
        PDP = (PageTable*)((u64)PDE.address << 12);
    }


    PDE = PDP->entries[indexer.pml3i];
    PageTable* PD;
    if (!PDE.present){
        PD = (PageTable*)pmm_alloc_block();
        memset(PD, 0, 0x1000);
        PDE.address = (u64)PD >> 12;
        PDE.present = true;
        PDE.rw = true;
        PDP->entries[indexer.pml3i] = PDE;
    }
    else
    {
        PD = (PageTable*)((u64)PDE.address << 12);
    }

    PDE = PD->entries[indexer.pml2i];
    PageTable* PT;
    if (!PDE.present){
        PT = (PageTable*)pmm_alloc_block();
        memset(PT, 0, 0x1000);
        PDE.address = (u64)PT >> 12;
        PDE.present = true;
        PDE.rw = true;
        PD->entries[indexer.pml2i] = PDE;
    }
    else
    {
        PT = (PageTable*)((u64)PDE.address << 12);
    }

    PDE = PT->entries[indexer.pml1i];
    PDE.address = (u64)phys_addr >> 12;
    PDE.present = true;
    PDE.rw = true;
    PT->entries[indexer.pml1i] = PDE;

    invalidate_tlb();

    return SUCCESS;
} /* vmm_map */


static PageTable * get_pml4_address(){
    u64 cr3;
    asm volatile ("mov %%cr3, %%rax" : "=a" (cr3));
    return (PageTable *) cr3;
}

i32 vmm_init(){
    gp_pml4 = get_pml4_address();        

    kprintf("[VMM]  PML4 located at 0x%x\n", (u64)gp_pml4);
    kprintf("[VMM]  Initialized paging\n");

    return SUCCESS;
} /* vmm_init */


