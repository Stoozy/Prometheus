#include "../kprintf.h"
#include "../typedefs.h"
#include <stdbool.h>
#include "../string/string.h"

#include "vmm.h"
#include "pmm.h"

#define PAGE_SIZE   4096
#define SUCCESS     1

extern void load_pagedir(PML4 * p_pml4);
extern void invalidate_tlb();

volatile PML4 pml4 = {};

volatile PageIndex page_index;

PageIndex vmm_get_index(u64 vaddr){

    vaddr >>= 12;
    page_index.pml1i = vaddr & 0x1ff;

    vaddr >>= 9;
    page_index.pml2i = vaddr & 0x1ff;

    vaddr >>= 9;
    page_index.pml3i = vaddr & 0x1ff;

    vaddr >>= 9;
    page_index.pml4i = vaddr & 0x1ff;

    return page_index;
}

i32 vmm_map(void * p_virtual, void* p_physical){
    PageIndex page_index = vmm_get_index((u64)p_virtual);
    
    Pml4e pml4e = pml4.entries[page_index.pml4i];

    PML3 * p_pml3;
    if(!pml4e.present){
        p_pml3 = (PML3*)pmm_alloc_block();

        memset((void*)p_pml3, 0x0, PAGE_SIZE); /* empty entries */
        pml4e.physical_address = (u64)p_pml3 >> 12;

        /* setup pml4 entry */
        pml4e.present = 1;
        pml4e.writable = 1;
        pml4e.user = 0;

        /* set pml4 address */
        pml4.entries[page_index.pml4i] = pml4e;
    }else p_pml3 = (PML3 *)((u64)pml4e.physical_address << 12); /*logical address*/

    Pml3e pml3e = p_pml3->entries[page_index.pml3i];
    PML2 * p_pml2;
    if(!pml3e.present){
        p_pml2 = (PML2*)pmm_alloc_block();

        memset((void*)p_pml2, 0x0, PAGE_SIZE); /* empty entries */

        /* setup pml3 entry */
        pml3e.physical_address = (u64)p_pml2 >> 12;
        pml3e.present = 1;
        pml3e.writable = 1;
        pml3e.user = 0;

        /* set pml3 address */
        p_pml3->entries[page_index.pml3i] = pml3e;
    }else p_pml2 = (PML2 *)((u64)pml3e.physical_address << 12); /*logical address*/

    Pml2e pml2e = p_pml2->entries[page_index.pml2i];
    PML1 * p_pml1;
    if(!pml2e.present){
        p_pml1 = (PML1*)pmm_alloc_block();

        memset((void*)p_pml1, 0x0, PAGE_SIZE); /* empty entries */

        /* setup pml2 entry */
        pml2e.physical_address = (u64)p_pml1 >> 12;
        pml2e.present = 1;
        pml2e.writable = 1;
        pml2e.user = 0;

        p_pml2->entries[page_index.pml2i] = pml2e;
    }else p_pml1 = (PML1*)((u64)pml2e.physical_address << 12 ); /* logical address */

    Pml1e pml1e = p_pml1->entries[page_index.pml1i];

    /* setup pml1 entry */
    pml1e.physical_address = (u64)p_physical >> 12;
    pml1e.present = 1;
    pml1e.writable = 1;
    pml1e.user = 0;

    return SUCCESS;
}

i32 vmm_init(){
    /* clear all entries */
    memset((void*)&pml4, 0x0, sizeof(pml4));

    /* id maps 8 gib*/
    for(u64 addr = 0x0; addr < 0x200000000; ++addr){ 
        vmm_map((void*)addr, (void*)addr);
        addr += 0x1000;
    }

    /*asm ("mov %0, %%cr3" : : "r" (pml4));*/
    load_pagedir((PML4*)&pml4);
    kprintf("[VMM]  Initialized paging\n");

    return SUCCESS;
}


