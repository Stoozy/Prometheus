#include "kmalloc.h"
#include "kprintf.h"
#include "liballoc.h"
#include "memory/pmm.h"
#include "string/string.h"

extern void acquire_lock(volatile u32*);
extern void release_lock(volatile u32*);

volatile u32 lock = 0;   // initiallly free

int liballoc_lock(){
    acquire_lock(&lock);
    return 0;
}

int liballoc_unlock(){
    release_lock(&lock);
    return 0;
}

void * liballoc_alloc(int pages){
    return pmm_alloc_blocks(pages);
}

int   liballoc_free(void* addr, int blocks){
    if(blocks == 1)
        pmm_free_block((u64)addr);
    else pmm_free_blocks((u64)addr, blocks);
    return 0;
}
