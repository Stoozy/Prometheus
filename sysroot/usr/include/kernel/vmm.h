#include <kernel/paging.h>

#define PAGE_SIZE   4096
#define PAGES_PER_TABLE 1024
#define TABLES_PER_DIR 1024


typedef uint32_t virt_addr;

bool vmm_alloc_page(page_t * e);
void vmm_free_page (page_t * e);
void vmm_flush_tlb_entry (virt_addr addr);
void vmm_map_page (void* phys, void* virt);
void vmm_init ();
page_t* vmm_ptable_lookup_entry (page_table_t* p, virt_addr addr);
page_dir_t* vmm_get_directory(); 

