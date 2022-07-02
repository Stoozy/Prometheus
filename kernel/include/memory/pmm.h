#ifndef _PMM_H 
#define _PMM_H

#include <typedefs.h>
#include <stddef.h>
#include <stdbool.h>

#define PMM_BLOCK_SIZE          4096
#define PMM_BLOCKS_PER_BYTE     8
#define PMM_MAX_BITMAPS         262144 /* up to 64 GiB memory (Takes up 2MiB) */


void        pmm_init();

void *      pmm_alloc_block();
void *      pmm_alloc_blocks(u64 blocks);
void        pmm_free_block(u64 addr);
void        pmm_free_blocks(u64 addr, u64 blocks);

u64         pmm_get_free_block_count();
u64         pmm_get_block_count();

void        pmm_dump();

#endif

