#pragma once

#define PAGE_SIZE   4096
#define PAGES_PER_TABLE 1024
#define TABLES_PER_DIR 1024

typedef uint32_t virt_addr;
typedef uint32_t pte_t;
typedef uint32_t pde_t;

typedef pte_t   page_tab_t[PAGES_PER_TABLE] __attribute__((aligned(4096)));
typedef pde_t   page_dir_t[TABLES_PER_DIR] __attribute__((aligned(4096)));



bool pte_set_frame(pte_t * e, phys_addr );
void pte_add_attrib(pte_t * e, uint32_t );
void pte_del_attrib(pte_t * e, uint32_t );

bool vmm_alloc_page(pte_t * e);
void vmm_free_page (pte_t * e);
void vmm_flush_tlb_entry (virt_addr addr);
void vmm_map_page (void* phys, pte_t * virt);
void vmm_init();

pde_t* vmm_ptable_lookup_entry (page_tab_t* p, virt_addr addr);
page_dir_t* vmm_get_directory(); 

