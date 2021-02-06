#include <kernel/vmm.h>
#include <kernel/pmm.h>
#include <kernel/util.h>
#include <string.h>
#include <stddef.h>

#define PAGE_ENTRIES        1024
#define PAGE_SIZE           4096
#define PAGES_PER_BYTE      8
#define MAX_BITMAPS         131072

extern void kernel_panic();
extern void load_page_directory(uint32_t *);
extern void init_paging();


static inline void invlpg(void* m)
{
    asm volatile ( "invlpg (%0)" : : "b"(m) : "memory" );
}

bool pte_set_frame(pte_t * e, phys_addr pa){
    *e =  pa | 3;
}


bool vmm_alloc_page(pte_t * e);
void vmm_free_page (pte_t * e);

void vmm_flush_tlb_entry (virt_addr addr){
   invlpg((void *) addr);
}

void vmm_map_page (void* phys, pte_t * virt){
    pte_set_frame(virt, (phys_addr)phys);
}

void vmm_init(){
    pde_t * dir = (pde_t*)pmm_alloc_blocks(8);
    pte_t * first_tab = (pte_t*)pmm_alloc_blocks(8);

    if(dir && first_tab){
        printf("Page Directory is located at 0x%x and first table is at 0x%x\n", dir, first_tab);
    } else kernel_panic("Paging Failed");


    // id map 4MiB
    uint32_t i=0, addr = 0; 
    for(; i<1024; i++){
        //first_tab[i] = (i*PAGE_SIZE) | 3;
        first_tab[i] = addr | 3;
        addr += PAGE_SIZE; 
    }

    dir[0] = ((uint32_t)first_tab) | 3;

    for(i=1; i<1022; i++){
        // 8 * 4096 bytes will let us store 1024 32 byte entries
        uint32_t * tab = (uint32_t*)pmm_alloc_blocks(8);
        for(pte_t j=0; j<PAGE_ENTRIES; j++){
            phys_addr p = (phys_addr)pmm_alloc_block();
            //tab[j] = addr | 3;
            tab[j] =  p | 3;
            //addr += PAGE_SIZE;
        }
        dir[i] = ((uint32_t)tab) | 3;
    }

    load_page_directory(dir);
    init_paging();
}

