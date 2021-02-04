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

void vmm_init(){
    uint32_t * dir = (uint32_t*)pmm_alloc_blocks(8);
    uint32_t * first_tab = (uint32_t*)pmm_alloc_blocks(8);

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
        uint32_t * tab = (uint32_t*)pmm_alloc_blocks(8);
        for(uint32_t j=0; j<PAGE_ENTRIES; j++){
            //uint32_t *ptr = (uint32_t*) pmm_alloc_block();
            //if(!ptr)  kernel_panic("Paging Failed\n");
            //tab[j] = ((uint32_t)ptr) | 3; 
            tab[j] = addr | 3;
            addr += PAGE_SIZE;
        }
        dir[i] = ((uint32_t)tab) | 3;
    }

    load_page_directory(dir);
    init_paging();
}
