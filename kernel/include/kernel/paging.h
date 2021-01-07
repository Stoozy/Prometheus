#include <kernel/typedefs.h>

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
}page_directory_entry_t;

typedef struct page_table_entry{
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
} page_table_entry_t;

typedef struct page_table{
    page_table_entry_t table[1024];
} __attribute__ ((aligned(4096))) page_table_t;


typedef struct page_drectory{
    page_table_t directory[1024];
} __attribute__((aligned(4096))) page_directory_t;








