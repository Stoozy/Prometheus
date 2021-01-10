#include <kernel/typedefs.h>

typedef uint32_t phys_addr;
void pmm_init(size_t mem_size, phys_addr bitmap);

void pmm_init_region(phys_addr base, size_t size);
void pmm_destroy_region(phys_addr  base, size_t size);

int pmm_get_first_free();
void * pmm_alloc_block();

void pmm_free_block(void * ptr);

uint32_t pmm_get_block_count();

