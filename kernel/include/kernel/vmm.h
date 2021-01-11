#include <kernel/paging.h>

typedef uint32_t virt_addr;

bool vmm_alloc_page(page_t * e);
void vmm_free_page (page_t * e);
inline pt_entry* vmm_ptable_lookup_entry (page_table_t* p, virt_addr addr);
page_dir_t* vmm_get_directory(); 
void vmm_flush_tlb_entry (virt_addr addr);

void vmm_map_page (void* phys, void* virt);

void vmm_init ();
