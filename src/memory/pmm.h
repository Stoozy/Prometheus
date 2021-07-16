#ifndef _PMM_H 
#define _PMM_H

#include <typedefs.h>
#include <stddef.h>

void        pmm_init();
void        pmm_init_region(void * addr, size_t size);
void *      pmm_alloc_block();
void *      pmm_alloc_blocks(u32 blocks);
void        pmm_free_block(u32 addr);
void        pmm_free_blocks(u32 addr, u32 blocks);
u32    pmm_get_free_block_count();
u32    pmm_get_block_count();
void        pmm_dump();

#endif

