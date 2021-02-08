#pragma once

#include <kernel/typedefs.h>
#include <stdbool.h>
#include <stddef.h>

#define PAGE_SIZE   4096
#define PAGES_PER_TABLE 1024
#define TABLES_PER_DIR 1024

typedef uint32_t virt_addr;
typedef uint32_t phys_addr;

typedef uint32_t pte_t;
typedef uint32_t pde_t;


typedef struct node {
    virt_addr start;
    size_t size;
    struct node * next;
} node_t;


void vmm_dump_list();

bool pte_set_frame(pte_t * e, phys_addr );
void pte_add_attrib(pte_t * e, uint32_t );
void pte_del_attrib(pte_t * e, uint32_t );

void * vmm_alloc_page();
void vmm_free_page (virt_addr , size_t);
void vmm_flush_tlb_entry (void * addr);
void vmm_map_page (void* phys, pte_t * virt);
void vmm_init();

//pde_t* vmm_ptable_lookup_entry (page_tab_t* p, virt_addr addr);
//page_dir_t* vmm_get_directory(); 

