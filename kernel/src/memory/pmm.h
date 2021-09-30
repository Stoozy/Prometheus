#ifndef _PMM_H 
#define _PMM_H

#include "../typedefs.h"
#include <stddef.h>
#include <stdbool.h>

#define _PMM_BLOCK_SIZE          4096
#define _PMM_BLOCKS_PER_BYTE     8
#define _PMM_MAX_BITMAPS         262144 /* up to 64 GiB memory */ 


void        pmm_init();

void        pmm_init_region(void * addr, u64 size);
void *      pmm_alloc_block();
void *      pmm_alloc_blocks(u64 blocks);
void        pmm_mark_region_used(void* start, void* end);

void        pmm_free_block(u64 addr);
void        pmm_free_blocks(u64 addl, u64 blocks);

u64         pmm_get_free_block_count();
u64         pmm_get_block_count();

bool        pmm_is_block_free(u64 block);
void        pmm_dump();

#endif

