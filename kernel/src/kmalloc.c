#include "kmalloc.h"
#include "kprintf.h"
#include "liballoc.h"
#include "memory/pmm.h"
#include "string/string.h"

int   liballoc_lock(){
    asm volatile ("cli");
    return 0;
}

int   liballoc_unlock(){
    asm volatile ("sti");
    return 0;
}

void* liballoc_alloc(int pages){
    return pmm_alloc_blocks(pages);
}

int   liballoc_free(void* addr,int blocks){
    pmm_free_blocks((u64)addr, blocks);
    return 0;
}
