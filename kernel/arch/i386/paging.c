#include <kernel/paging.h>
#include <kernel/util.h>

#include <stdbool.h>

void pte_add_attrib (page_t* e, uint32_t attrib){
    _set_bit(e, attrib);
}

void pte_del_attrib (page_t* e, uint32_t attrib){
    _clear_bit(e, attrib);
}
void pte_set_frame (page_t * e, phys_addr addr){
    e->phys_addr = addr;
}

phys_addr pte_get_phys_addr (page_t *e){
    if(!(e->present)) return 0; // not present in physical mem
    return e->phys_addr;
}

bool pte_is_present(page_t * e){
    if(e->present) return true;
    return false;
}

bool pte_is_writable(page_t * e){
    if(e->rw) return true;
    return false;
}

void pde_add_attrib (pde_t* e, uint32_t attrib){
    _set_bit(e, attrib);
}

void pde_del_attrib (pde_t* e, uint32_t attrib){
    _clear_bit(e, attrib);
}

void pde_set_frame (pde_t* e, phys_addr addr){
    e->addr = addr;
}

bool pde_is_present (pde_t * e){
    if(e->present) return true;
    return false;
}
bool pde_is_user (pde_t * e){
    if(e->user) return true;
    return false;
}
bool pde_is_4mb (pde_t * e){
    if(e->size) return true;    // 4 MiB
    else return false;          // 4 KiB  
}
bool pde_is_writable (pde_t  *e){
    if(e->rw) return true;  // rw
    return false;           // read only
}
phys_addr   pde_get_phys_addr (pde_t *e){
    if(!(e->present)) return 0;
    return e->addr;
}
void pde_enable_global (pde_t *e){
    return;
}



