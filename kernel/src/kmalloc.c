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

//volatile void * current_address; 
//volatile u64 KMEM_MAX;
//
//void kmalloc_init(u64 mem_size){
//    current_address = pmm_alloc_blocks(mem_size / _PMM_BLOCK_SIZE);
//    KMEM_MAX = (u64)current_address+mem_size;
//
//}
//
//
//void * kmalloc(size_t size){
//    if((u64)(current_address+size) > KMEM_MAX){
//        kprintf("[KMALLOC]  OUT OF MEMORY!\n");
//        while(1); // crash
//    }
//    
//    current_address += size;
//    kprintf("[KMALLOC] Allocated %d bytes\n", size);
//    memset((void*)current_address-size, 0x0, size);
//    return (void*)(current_address-size); /* return address before it was changed */
//}
