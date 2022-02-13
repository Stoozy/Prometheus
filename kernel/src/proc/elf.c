#include "elf.h"
#include "../typedefs.h"
#include "../memory/pmm.h"
#include "../memory/vmm.h"
#include "../string/string.h"
#include "../proc/proc.h"
#include "../kmalloc.h"

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

    //if(elf[4] == 1){
        /* 32 bit elf file */
        //return load_elf_32(elf);
    //}else{
        return load_elf_64(elf);
    //}
}

u8 load_elf_64(uint8_t * elf){
    Elf64_Ehdr * elf64 = (Elf64_Ehdr *) elf;

    ProcessControlBlock * elf_pcb = kmalloc(sizeof(ProcessControlBlock));

    elf_pcb->p_stack = kmalloc(0x1000) +  0x1000;
    PageTable * elf_cr3 = vmm_create_user_proc_pml4(elf_pcb->p_stack);


    for(u64 segment=0; segment<elf64->e_phnum; ++segment){
        Elf64_Phdr * p_header = (Elf64_Phdr *) 
            (elf + elf64->e_phoff + (elf64->e_phentsize * segment));

        if(p_header->p_type == PT_LOAD){
            /* load segment here */

            /* "Allocate Memory" */
            for(u64 page=p_header->p_vaddr/PAGE_SIZE; 
                    page<(p_header->p_vaddr+p_header->p_memsz)/PAGE_SIZE+20; ++page)
            {
                void * phys_addr = pmm_alloc_block();
                int flags = PAGE_USER | PAGE_READ_WRITE | PAGE_PRESENT;
                vmm_map(elf_cr3, (void*)(page*PAGE_SIZE), phys_addr, flags);
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

	u64 * stack = (u64 *)(elf_pcb->p_stack);
    // user proc
	*--stack = 0x23; // ss
	*--stack = (u64)elf_pcb->p_stack; // rsp
	*--stack = 0x202 ; // rflags
	*--stack = 0x2b; // cs
	*--stack = (u64)entrypoint; // rip

    *--stack = 0; // r8
    *--stack = 0;
    *--stack = 0; 
    *--stack = 0; // ...
    *--stack = 0; 
    *--stack = 0;
    *--stack = 0;
    *--stack = 0; // r15


	*--stack = (u64)elf_pcb->p_stack; // rbp

	*--stack = 0; // rdx
	*--stack = 0; // rsi
	*--stack = 0; // rdi

    elf_pcb->p_stack = stack;
    elf_pcb->cr3 = elf_cr3;
    elf_pcb->next = NULL;

    kprintf("ELF entrypoint %x\n", entrypoint );
    kprintf("ELF cr3 %x\n", elf_cr3);

    register_process(elf_pcb);

#ifdef SCHEDULER_DEBUG
    dump_list();
#endif

    return 0;
}


