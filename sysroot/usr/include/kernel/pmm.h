#include <kernel/typedefs.h>
#include <stddef.h>
#include <stdbool.h>

typedef uint32_t phys_addr;
void pmm_init(size_t mem_size);

void pmm_init_region(phys_addr base, uint64_t size);
void pmm_destroy_region(phys_addr  base, size_t size);

int pmm_get_first_free();
void * pmm_alloc_block();
void * pmm_alloc_blocks(uint32_t blocks);

void pmm_free_block(void * ptr);
void pmm_load_PDBR(phys_addr addr);

uint32_t pmm_get_block_count();
uint32_t pmm_get_free_block_count();
void pmm_paging_enable(bool b);



