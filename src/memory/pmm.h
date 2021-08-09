#ifndef _PMM_H 
#define _PMM_H

#include "../typedefs.h"
#include <stddef.h>

void        pmm_init();

void        pmm_init_region(void * addr, u64 size);
void *      pmm_alloc_block();
void *      pmm_alloc_blocks(u64 blocks);
void        pmm_mark_region_used(void *  start_addr, void* end_addr );

void        pmm_free_block(u64 addr);
void        pmm_free_blocks(u64 addl, u64 blocks);

u64         pmm_get_free_block_count();
u64         pmm_get_block_count();

void        pmm_dump();

#endif

