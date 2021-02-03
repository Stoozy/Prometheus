#include <kernel/vmm.h>
#include <kernel/pmm.h>
#include <kernel/util.h>
#include <string.h>
#include <stddef.h>

#define PAGE_SIZE           4096
#define PAGES_PER_BYTE      8
#define MAX_BITMAPS         131072

extern void kernel_panic();
extern void load_page_directory(uint32_t *);
extern void init_paging();

void vmm_init(){

}

