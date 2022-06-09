#include "kmalloc.h"
#include "kprintf.h"
#include "liballoc.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "string/string.h"

#define PAGING_VIRTUAL_OFFSET       0xffff800000000000

extern void acquire_lock(volatile u32*);
extern void release_lock(volatile u32*);

volatile u32 lock = 0;   // initiallly free

extern void invalidate_tlb();
extern void load_pagedir(PageTable *);

void print_spin_wait(){
    kprintf("[ALLOCATOR SPINLOCK]   Spinlock waiting...\n");
}

int liballoc_lock(){
    acquire_lock(&lock);
    return 0;
}

int liballoc_unlock(){
    release_lock(&lock);
    return 0;
}

void * liballoc_alloc(int pages){

    kprintf("Got pages %d\n", pages);

    if(pages < 0) {
        return NULL;
    }

    void * addr = pmm_alloc_blocks(pages);

    PageTable * current_cr3 = vmm_get_current_cr3();

    int flags = PAGE_WRITE | PAGE_PRESENT;
    for(u64 page = 1; page<pages; ++page){
        void * current_addr = addr + (page * PAGE_SIZE);

#ifdef ALLOCATOR_DEBUG
        kprintf("[ALLCOATOR]    Identity mapping 0x%llx\n", current_addr);
#endif
        vmm_map(current_cr3, current_addr, current_addr-PAGING_VIRTUAL_OFFSET,  flags);
    }

    //invalidate_tlb();
    load_pagedir(current_cr3);
    return addr;
}

int   liballoc_free(void * addr, int pages){
    if(pages == 1)
        pmm_free_block((u64)addr);
    else pmm_free_blocks((u64)addr, pages);
    return 0;
}
