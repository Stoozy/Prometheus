#ifndef _VMM_H
#define _VMM_H 1

#include "../typedefs.h"
#include <stdbool.h>

typedef struct {
    u64 pml4i;
    u64 pml3i;
    u64 pml2i;
    u64 pml1i;
} __attribute__((packed)) PageIndex;

typedef struct {
    bool present: 1;
    bool rw: 1;
    bool user: 1;
    bool writethrough: 1;
    bool cache_disabled: 1;
    bool accessed : 1;
    bool zero0 : 1;
    bool size : 1;
    bool zero1: 1;
    u8 available : 3;
    u64 address: 52;
} __attribute__((packed)) PageTableEntry;


typedef struct {
    PageTableEntry entries[512];
} __attribute__((packed)) PageTable;

i32 vmm_init();
i32 vmm_map(void * p_virtual, void * p_physical);
PageIndex vmm_get_page_index(u64 vaddr);

#endif 
