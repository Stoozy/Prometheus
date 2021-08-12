#include "kmalloc.h"
#include "kprintf.h"

volatile u64 current_address = _KMALLOC_ADDRESS_BEGIN;

void * kmalloc(size_t size){
    if(current_address+size > _KMALLOC_ADDRESS_LIMIT){
        kprintf("[KMALLOC]  OUT OF MEMORY!");
        return 0x0;
    }
    
    current_address += size;
    return (void*)(current_address-size);
}
