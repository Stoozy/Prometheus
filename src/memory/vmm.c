#include <kprintf.h>
#include <typedefs.h>
#include <stdbool.h>

#include "vmm.h"
#include "pmm.h"

#define PAGE_SIZE   4096

extern void load_pagedir(struct Pml4e * p_pml4);
extern void invalidate_tlb();

volatile struct PML4 kpml4 = {};
volatile struct PML3 kpml3 = {};
volatile struct PML2 kpml2 = {};
volatile struct PML1 kpml1[512] = {};


i32 vmm_init(){

    struct Pml4e * p_pml4e = &kpml4.entries[0];
    p_pml4e->present = 1;
    p_pml4e->writable = 1;
    p_pml4e->user = 0;
    p_pml4e->physical_address = (u64)&kpml3 / PAGE_SIZE;


    struct Pml3e * p_pml3e = &kpml3.entries[0];
    p_pml3e->present = 1;
    p_pml3e->writable = 1;
    p_pml3e->user = 0;
    p_pml3e->physical_address = (u64)&kpml2 / PAGE_SIZE;
    p_pml3e->size = 0;

    for(u16 i = 0; i<512; ++i){
        struct Pml2e * p_pml2e = &kpml2.entries[i];
        p_pml2e->present = 1;
        p_pml2e->writable = 1;
        p_pml2e->user = 0;
        p_pml2e->physical_address = (u64)&kpml1[i] / PAGE_SIZE;
    }

    kprintf("[VMM]  Setting pml1 table\n");

    u64 addr = &kpml1 + (512 * sizeof(u64)); 
    for(u16 i = 0; i<512; ++i){
        for(u16 j = 0; j<512; ++j){
            kpml1[i].entries[j].user = 0;
            kpml1[i].entries[j].present = 1;
            kpml1[i].entries[j].writable = 1;
            kpml1[i].entries[j].physical_address = addr / PAGE_SIZE; 
            addr += 0x1000;
            /* kprintf("Virtual address: 0x%x\n", addr);*/
        }
    }

    invalidate_tlb();
    load_pagedir(&kpml4);

    kprintf("[VMM]  Initialized paging\n");
}



