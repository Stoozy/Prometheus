#include "elf.h"
#include "proc.h"

#include "../typedefs.h"
#include "../memory/pmm.h"
#include "../memory/vmm.h"
#include "../string/string.h"
#include "../proc/proc.h"
#include "../kmalloc.h"
#include "../fs/vfs.h"
#include "../kprintf.h"
#include "../config.h"


extern void load_pagedir(PageTable*);


u8 validate_elf(u8 * elf) {
    if(elf[0] != 0x7f 
        || elf[1] != 'E'
        || elf[2] != 'L'
        || elf[3] != 'F')
    {
        /* invalid elf header */
        return 0;
    }

    return 1; 
}


#define LD_BASE 0xC000000

ElfInfo load_elf_segments(PageTable * vas, uint8_t * elf_data, bool interp){
    kprintf("Load elf segments called with %x vas and %x elf buffer interp: %d\n", vas, elf_data, interp);
    ElfInfo info = {0};

    // load elf file here

    Elf64_Ehdr * elf_hdr = (Elf64_Ehdr *) elf_data;

    for(u64 segment=0; segment<elf_hdr->e_phnum; ++segment){
        Elf64_Phdr * p_header = (Elf64_Phdr *) 
            (elf_data + elf_hdr->e_phoff + (elf_hdr->e_phentsize * segment));

        if(p_header->p_type ==  PT_INTERP){
            char * ld_path = kmalloc(p_header->p_memsz);
            memset(ld_path, 0, p_header->p_memsz);
            memcpy(ld_path, (elf_data+p_header->p_offset), p_header->p_filesz);
            kprintf("[ELF]  Got interpreter file path: %s\n", ld_path);
            FILE * ld_file = vfs_open(ld_path, 0);
            u8 * ld_data = kmalloc(ld_file->size);
            int br = vfs_read(ld_file, ld_file->size, ld_data);

            continue;
            if(br != 0 && validate_elf(ld_data))
                info.entrypoint = load_elf_segments(vas, ld_data, true).entrypoint;
        }
		
        if(p_header->p_type != PT_LOAD) continue;

        size_t offset = p_header->p_vaddr & (PAGE_SIZE-1);
        size_t blocks = (p_header->p_memsz/PAGE_SIZE)+1;

        void * phys_addr = pmm_alloc_blocks(blocks);

        memset(phys_addr, 0, p_header->p_memsz);
        memcpy(phys_addr+offset, (elf_data+p_header->p_offset), p_header->p_memsz);

        void * virt_addr = interp ? (void*)(LD_BASE + p_header->p_vaddr) : (void*)p_header->p_vaddr;

        int page_flags = PAGE_USER | PAGE_READ_WRITE | PAGE_PRESENT;
        for(int block=0; block<blocks; block++){
            vmm_map(vas, virt_addr, phys_addr, page_flags); 
            virt_addr += PAGE_SIZE; phys_addr += PAGE_SIZE;
        }
	}

    return info;
}

ProcessControlBlock * create_elf_process(const char * path){
    FILE * elf_file = vfs_open(path, 0);
    u8 * elf_data = kmalloc(elf_file->size);
    int br = vfs_read(elf_file, elf_file->size, elf_data);

    if(!validate_elf(elf_data))
        return NULL;


	ProcessControlBlock * proc = kmalloc(sizeof(ProcessControlBlock));

    PageTable * vas = (PageTable*)pmm_alloc_block(); 
    proc->cr3 = vas;
    memset(proc->cr3, 0, PAGE_SIZE);

    ElfInfo info = load_elf_segments(proc->cr3, elf_data, false);
 
    /* 4 KiB stack */
    proc->p_stack = pmm_alloc_blocks(4) + 4 * PAGE_SIZE;

	u64 * stack = (u64 *)(proc->p_stack);
    *--stack = 0x23; // ss
	*--stack = (u64)proc->p_stack; // rsp
	*--stack = 0x202 ; // rflags
	*--stack = 0x2b; // cs
	*--stack = (u64)info.entrypoint; // rip

    *--stack = 0; // r8
    *--stack = 0;
    *--stack = 0; 
    *--stack = 0; // ...
    *--stack = 0; 
    *--stack = 0;
    *--stack = 0;
    *--stack = 0; // r15
	*--stack = 0; // rbp
	*--stack = 0; // rdx
	*--stack = 0; // rsi
	*--stack = 0; // rdi

    proc->p_stack = stack;
    proc->next = NULL;

    return proc;
}


