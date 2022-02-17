#include "elf.h"
#include "proc.h"

#include "../typedefs.h"
#include "../memory/pmm.h"
#include "../memory/vmm.h"
#include "../string/string.h"
#include "../proc/proc.h"
#include "../kmalloc.h"

#include "../kprintf.h"
#include "../config.h"


extern void load_pagedir(PageTable*);

u8 load_elf_64(  u8 * elf);

u8 load_elf_bin( u8 * elf) {
    if(elf[0] != 0x7f 
        || elf[1] != 'E'
        || elf[2] != 'L'
        || elf[3] != 'F')
    {
        /* invalid elf header */
        return -1;
    }

    //if(elf[4] == 1){
        /* 32 bit elf file */
        //return load_elf_32(elf);
    //}else{
        return load_elf_64(elf);
    //}
}

u8 load_elf_64( u8 * elf){
    Elf64_Ehdr * elf64 = (Elf64_Ehdr *) elf;

    ProcessControlBlock * proc = create_process((void*)elf64->e_entry);

    for(u64 segment=0; segment<elf64->e_phnum; ++segment){
        Elf64_Phdr * p_header = (Elf64_Phdr *) 
            (elf + elf64->e_phoff + (elf64->e_phentsize * segment));

        if(p_header->p_type == PT_LOAD){
            /* found loadable segment */

            int flags = PAGE_USER | PAGE_READ_WRITE | PAGE_PRESENT;

            u64 blocks = ((p_header->p_vaddr+p_header->p_filesz)/PAGE_SIZE - p_header->p_vaddr/PAGE_SIZE) +1;
            void * phys_addr = pmm_alloc_blocks(blocks);

            kprintf("Found loadable segment at offset 0x%x\n", p_header->p_offset);
            memset(phys_addr, 0, p_header->p_memsz);
            memcpy(phys_addr, 
                    (void*)elf+(p_header->p_offset), p_header->p_filesz);

            // now map those pages 
            void* virt_addr =  (void*)p_header->p_vaddr;

            for(u64 page = 0; page<blocks; ++page){
                vmm_map(proc->cr3, virt_addr, phys_addr+(page*PAGE_SIZE), flags);
                virt_addr += PAGE_SIZE;
            }
        }
    }

    /* At this point, the elf program segments are loaded into memory
     * Now just create a process with the entry point and register it... 
     */

    kprintf("ELF entrypoint %x\n", elf64->e_entry);
    kprintf("ELF cr3 %x\n", proc->cr3);

    register_process(proc);

#ifdef SCHEDULER_DEBUG
    dump_list();
#endif

    return 0;
}


