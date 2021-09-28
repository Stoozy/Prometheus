#include "elf.h"
#include "../typedefs.h"
#include "../memory/pmm.h"
#include "../memory/vmm.h"
#include "../string/string.h"
#include "../proc/tasking.h"

#include "../kprintf.h"
#include "../config.h"

u8 load_elf_64(u8 * elf);
u8 load_elf_32(u8 * elf);

u8 load_elf_bin(u8 * elf) {
    if(elf[0] != 0x7f 
        || elf[1] != 'E'
        || elf[2] != 'L'
        || elf[3] != 'F')
    {
        /* invalid elf header */
        return -1;
    }

    if(elf[4] == 1){
        /* 32 bit elf file */
        return load_elf_32(elf);
    }else{
        return load_elf_64(elf);
    }
}

u8 load_elf_64(u8 * elf){
    Elf64_Ehdr * elf64 = (Elf64_Ehdr *) elf;

    for(u64 segment=0; segment<elf64->e_phnum; ++segment){
        Elf64_Phdr * p_header = (Elf64_Phdr *) 
            (elf + elf64->e_phoff + (elf64->e_phentsize * segment));

        if(p_header->p_type == PT_LOAD){
            /* load segment here */

            /* "Allocate Memory" */
            for(u64 page=p_header->p_vaddr/PAGE_SIZE; 
                    page<(p_header->p_vaddr+p_header->p_memsz)/PAGE_SIZE +1; ++page)
            {
                void * phys_addr = pmm_alloc_block();
                vmm_map(vmm_get_current_cr3(), (void*)(page*PAGE_SIZE), phys_addr);
            }

            memset((void*)p_header->p_vaddr, 0, p_header->p_memsz);

            memcpy((void*)p_header->p_vaddr, 
                    (void*)elf+p_header->p_offset, p_header->p_memsz);
        }
    }

    /* At this point, the elf program segments are loaded into memory
     * Now just create a process with the entry point and register it... 
     */

    void (* entrypoint)(void) = (void*)elf64->e_entry;
    ProcessControlBlock * elf_pcb = create_process(entrypoint);

    register_process(elf_pcb);
#ifdef SMP_DEBUG
    dump_list();
#endif

    return 0;
}

u8 load_elf_32(u8 * elf){
    Elf32_Ehdr * elf32 = (Elf32_Ehdr*)elf;
    return 0;
}
