#pragma once
#include <kernel/typedefs.h>
#include <stddef.h>

void        pmm_init();
void        pmm_init_region(void * addr, size_t size);
void *      pmm_alloc_block();
void *      pmm_alloc_blocks(uint32_t blocks);
void        pmm_free_block(uint32_t addr);
void        pmm_free_blocks(uint32_t addr, uint32_t blocks);
uint32_t    pmm_get_free_block_count();
uint32_t    pmm_get_block_count();
void        pmm_dump();
