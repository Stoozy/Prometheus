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

    return SUCCESS;
} /* vmm_map */


i32 vmm_init(){
    gp_pml4 = (PageTable*) pmm_alloc_block();

    kprintf("[VMM]  PML4 Created at 0x%x\n", (u64)gp_pml4);
    /* clear all entries */
    memset((void*)gp_pml4, 0x0, sizeof(PageTable));
    //kprintf("[VMM]  PML4 cleared at 0x%x\n", (u64)gp_pml4);

    /* id maps 8 gib*/
    for(u64 addr = 0x0; addr < 0x200000000; addr+=0x1000){ 
        vmm_map((void*)addr, (void*)addr);
    }

    //for(u64 addr = 0x0; addr < 0x200000000; addr+=0x1000){ 
    //    vmm_map((void*)(0xffff800000000000+addr), (void*)addr);
    //}

    //for(u64 addr = 0x0; addr < 0x80000000; addr+=0x1000){ 
    //    vmm_map((void*)(0xffffffff80000000+addr), (void*)addr);
    //}

    //if(!gp_pml4->entries[0].present){
    //    kprintf("pml4e not present\n");
    //}else{
    //    kprintf("pml4e present\n");
    //    PageTable * p_pml3 = (PageTable*)((u64)gp_pml4->entries[0].address * PAGE_SIZE);
    //    if(!p_pml3->entries[0].present){
    //        kprintf("pml3e not present\n");
    //    }else{
    //        kprintf("pml3e present\n");
    //        PageTable * p_pml2 = (PageTable*)((u64)p_pml3->entries[0].address * PAGE_SIZE);
    //        if(!p_pml2->entries[0].present){
    //            kprintf("pml2e not present\n");
    //        }else{
    //            kprintf("pml2e present\n");
    //            PageTable * p_pml1 = (PageTable*)((u64)p_pml2->entries[0].address * PAGE_SIZE);
    //            if(!p_pml1->entries[0].present){
    //                kprintf("pml1e not present\n");
    //            }else{
    //                kprintf("pml1e present\n");
    //                kprintf("Pml1e address: 0x%x\n", p_pml1->entries[0].address);
    //            }
    //        }
    //    }
    //}
    
    load_pagedir(gp_pml4);
    //asm ("mov %0, %%cr3" : : "r" (gp_pml4));
    kprintf("[VMM]  Initialized paging\n");

    return SUCCESS;
} /* vmm_init */


