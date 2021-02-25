#include <kernel/liballoc.h>
#include <kernel/vmm.h>

int   liballoc_lock(){
    asm("cli");
    return 0;
}
int   liballoc_unlock(){
    asm("sti");
    return 0;
}
void * liballoc_alloc(int pages){
    return vmm_alloc_pages(pages);
}

int liballoc_free(void* addr,int pages){
    vmm_free_page((virt_addr)addr, pages*0x1000);
    return 0;
}
