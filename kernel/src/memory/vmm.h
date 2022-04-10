#ifndef _VMM_H
#define _VMM_H 1

#define PAGE_SIZE   4096

#include "../typedefs.h"
#include <stdbool.h>

enum {
    PAGE_PRESENT    = 1 << 0, // same as 1
    PAGE_READ_WRITE = 1 << 1, // same as 2, binary 10
    PAGE_USER       = 1 << 2, // same as 4, binary 100
};

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
    void * vaddr;
    void * paddr;
    u64 length;
} MemRange;


typedef struct {
    PageTableEntry entries[512];
} __attribute__((packed)) PageTable;

void *      vmm_virt_to_phys(PageTable * cr3, void * virt_addr);
PageIndex   vmm_get_page_index(u64 vaddr);
i32         vmm_map(PageTable * pml4, void * p_virtual, void * p_physical, int flags);
PageTable * vmm_create_user_proc_pml4();
PageTable * vmm_create_kernel_proc_pml4();
PageTable * vmm_get_current_cr3();
void        vmm_map_range(PageTable * cr3, MemRange range, int flags);
void        vmm_init();

#endif 
