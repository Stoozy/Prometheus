#include <kernel/vmm.h>
#include <kernel/pmm.h>
#include <kernel/util.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>

#define PAGE_ENTRIES            1024
#define PAGE_SIZE               4096
#define PAGES_PER_BYTE          8
#define MAX_BITMAPS             131072
#define PAGE_PHYS_ADDR          0xFFFFF000
#define PAGE_TABLE_ADDR         0xFFFFF000

extern void kernel_panic();
extern void load_page_directory(uint32_t *);
extern void init_paging();

// 7/8th of first page
static uint32_t placement_addr = 0x380000;

static node_t * freelist;

node_t * search(node_t * head, virt_addr search_addr){
    node_t * current = head;

    printf("Searching for address: 0x%x\n", search_addr);
    printf("Current :%x Search: %x\n", current->start, search_addr);
    if( current->start == search_addr)
        return current;

    while (current->next != NULL) {
        printf("Current :%x Search: %x\n", current->start, search_addr);
        if( current->start == search_addr)
            return current;
        current = current->next;
    }
    
    return 0;
}



void * kmalloc(size_t size){
    void * p = (void*)placement_addr;
    placement_addr+=size;
    if(placement_addr > 1024*1024*4)
        kernel_panic("Kernel Page list overflow");
    return p;
}

void * vmm_alloc_pages(int n){
    if(freelist->size >= n*PAGE_SIZE){
        // put start address at next page
        freelist->start += n*PAGE_SIZE;
        freelist->size -= n*PAGE_SIZE;
    } else kernel_panic("Out of memory!");
    return (void*)(freelist->start-(n*PAGE_SIZE));
}

void * vmm_alloc_page() {
    if(freelist->size != 0){
        // put start address at next page
        freelist->start += PAGE_SIZE;
        freelist->size -= PAGE_SIZE;
    } else kernel_panic("Out of memory!");

    // updated start address so subtract page_size
    return (void*)(freelist->start-PAGE_SIZE);
}

void vmm_free_page(virt_addr addr, size_t size){
    node_t * following = search(freelist, (virt_addr)addr+size);

    if(following != 0){
        following->start = addr;
        following->size += size;
    } else {
        printf("Pushing to free list: 0x%x size: %d\n", addr, size);
        push(freelist, addr, size);
    }     

    return;
}

void print_list(node_t * head) {
    node_t * current = head;

    while (head != NULL) {
        printf("0x%x: %u bytes => ", head->start, head->size);
        head = head->next;
    }
    printf("NULL\n");
    return;
}

void vmm_dump_list(){
    print_list(freelist);
    return;
}

void push(node_t * head, virt_addr start_addr, size_t size) {
    node_t * current = head;
    while (current->next != NULL) {
        current = current->next;
    }

    /* now we can add a new variable */
    current->next = (node_t *) kmalloc(sizeof(node_t));
    current->next->start = start_addr;
    current->next->size = size;
    current->next->next = NULL;
}


static inline void invlpg(void* m)
{
    asm volatile ( "invlpg (%0)" : : "b"(m) : "memory" );
}

bool pte_set_frame(pte_t * e, phys_addr pa){
    *e =  pa | 3;
}


void vmm_flush_tlb_entry (void * addr){
    invlpg( addr);
}

void vmm_map_page (void* phys, pte_t * virt){
    pte_set_frame(virt, (phys_addr)phys);
}

void vmm_init(){
    // initialize free page list, directories and tables 
    freelist = kmalloc(50*sizeof(node_t));

    pde_t * dir = (pde_t*) pmm_alloc_blocks(8);
    pte_t * first_tab = (pte_t*) pmm_alloc_blocks(8);

    if(dir && first_tab){
        printf("Page Directory is located at 0x%x and first table is at 0x%x\n", dir, first_tab);
    } else kernel_panic("Error occured setting up paging\n");


    // id map 4MiB
    uint32_t i=0, addr = 0; 
    for(; i<1024; i++){
        // map addres and mark present rw
        first_tab[i]  = addr | 3;
        addr += PAGE_SIZE; 
    }

    dir[0] = ((uint32_t)first_tab) | 3;

    for(i=2; i<1024; i++){
        // 8 * 4096 bytes will let us store 1024 32 byte entries
        uint32_t * tab = (uint32_t*)pmm_alloc_blocks(8);
        for(pte_t j=0; j<PAGE_ENTRIES; j++){
            phys_addr physaddr = (phys_addr)pmm_alloc_block();
            if(physaddr){
                // map page and mark present
                tab[j]  = physaddr | 3;
                addr += PAGE_SIZE;
                vmm_flush_tlb_entry (addr);
            }
        }
        dir[i] = ((uint32_t)tab) | 3;
    }


    load_page_directory(dir);
    init_paging();

    // push virtual addr to freelist
    // initially free from 4MiB- 4GiB
    freelist->start = 1024*1024*4;
    freelist->size = addr-freelist->start;

    printf("Initial free page list: \n");
    vmm_dump_list();
    
}
