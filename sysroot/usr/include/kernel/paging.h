#include <kernel/typedefs.h>
#include <kernel/pmm.h>

#include <stdbool.h>


typedef struct page_directory_entry{
    uint32_t present    :1; // in physical mem?
    uint32_t rw         :1; // 1: rw , 0: r only
    uint32_t user       :1; // 1: user access, 0: only kernel access 
    uint32_t write      :1; // 1: write through caching enabled, 0: write back is enabled. 
    uint32_t cache      :1; // 1: page won't be cached 0: page will be cached
    uint32_t accessed   :1; // 1: has been read/written to, 0:not read/written to
    uint32_t zero       :1;
    uint32_t size       :1; // 1: 4MiB, 0: 4KiB
    uint32_t ignored    :1;
    uint32_t avail      :3;
    uint32_t addr       :20; // page table 4-KiB aligned address 
} pde_t;

typedef struct page{
    uint32_t present    :1; // in physical mem?
    uint32_t rw         :1; // 1: rw , 0: r only
    uint32_t user       :1; // 1: user access, 0: only kernel access 
    uint32_t write      :1; // 1: write through caching enabled, 0: write back is enabled. 
    uint32_t cache      :1; // 1: page won't be cached 0: page will be cached
    uint32_t accessed   :1; // 1: has been read/written to, 0:not read/written to
    uint32_t dirty      :1; // 1: page has been written to, 0: not written to (cpu will not reset this)
    uint32_t zero       :1; // some shit called PAT idfk
    uint32_t global     :1;
    uint32_t avail      :3;
    uint32_t phys_addr  :20; // 4-KiB aligned physical address 
} page_t;


void        pte_add_attrib (page_t* e, uint32_t attrib);
void        pte_del_attrib (page_t* e, uint32_t attrib);
void        pte_set_frame (page_t * e, phys_addr);

bool        pte_is_present(page_t * e);
bool        pte_is_writable(page_t * e);

void        pde_add_attrib (pde_t* e, uint32_t attrib);
void        pde_t_del_attrib (pde_t* e, uint32_t attrib);
void        pde_t_set_frame (pde_t*, phys_addr);

bool        pde_t_is_present (pde_t e);
bool        pde_t_is_user (pde_t);
bool        pde_t_is_4mb (pde_t);
bool        pde_t_is_writable (pde_t e);
phys_addr   pde_t_pfn (pde_t e);
void        pde_t_enable_global (pde_t e);




