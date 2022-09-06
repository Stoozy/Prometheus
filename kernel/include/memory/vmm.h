#ifndef _VMM_H
#define _VMM_H 1

#define PAGE_SIZE   4096

#include <libk/typedefs.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PAGING_KERNEL_OFFSET        0xffffffff80000000
#define PAGING_VIRTUAL_OFFSET       0xffff800000000000


typedef struct process_control_block ProcessControlBlock;

enum {
    PAGE_PRESENT    = 1 << 0, // same as 1
    PAGE_WRITE      = 1 << 1, // same as 2, binary 10
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

typedef struct vas_range_node {
    void * virt_start;
    void * phys_start;
    size_t size;

    int page_flags;

    struct vas_range_node * next; 
} __attribute__((packed)) VASRangeNode;


typedef struct {
    PageTableEntry entries[512];
} __attribute__((packed)) PageTable;

void        vmm_map_page(PageTable * , uintptr_t , uintptr_t , int );
PageTable * vmm_create_user_proc_pml4(ProcessControlBlock *);
PageTable * vmm_create_kernel_proc_pml4(ProcessControlBlock *);
PageTable * vmm_get_current_cr3();

void        vmm_copy_vas(ProcessControlBlock*, ProcessControlBlock*);
void        vmm_switch_page_directory(PageTable *);
void        vmm_map_range(
                PageTable * cr3, 
                void * virt_start, 
                void * phys_start, 
                size_t size,
                int flags );


#endif 

