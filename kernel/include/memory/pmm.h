#pragma once

#include <libk/typedefs.h>
#include <stdbool.h>
#include <stddef.h>
#include <stivale2.h>

#define PMM_BLOCK_SIZE 4096
#define PMM_BLOCKS_PER_BYTE 8
#define PMM_MAX_BITMAPS 262144 /* up to 64 GiB memory (Takes up 2MiB) */

void pmm_init(struct stivale2_struct_tag_memmap *meminfo);

void *pmm_alloc_block();
void *pmm_alloc_blocks(size_t blocks);
void pmm_free_block(uintptr_t addr);
void pmm_free_blocks(uintptr_t addr, u64 blocks);

u64 pmm_get_free_block_count();
u64 pmm_get_block_count();

void pmm_dump();
