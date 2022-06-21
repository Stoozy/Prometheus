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


#define LD_BASE 0xA000000

Auxval load_elf_segments(PageTable * vas, u8 * elf_data){
    kprintf("[ELF]  Load elf segments called with %x vas and %x elf buffer\n", vas, elf_data);
    Auxval aux = {0};

    // load elf file here
    Elf64_Ehdr * elf_hdr = (Elf64_Ehdr *) elf_data;
    aux.entry = elf_hdr->e_entry;
    aux.phnum = elf_hdr->e_phnum;
    aux.phent = elf_hdr->e_phentsize;

    for(u64 segment=0; segment<elf_hdr->e_phnum; ++segment){
        Elf64_Phdr * p_header = (Elf64_Phdr *) 
            (elf_data + elf_hdr->e_phoff + (elf_hdr->e_phentsize * segment));

        if(p_header->p_type == PT_PHDR){
            kprintf("PT_PHDR vaddr is 0x%x\n", p_header->p_vaddr);
            aux.phdr = (u64)p_header->p_vaddr;
        }

        if(p_header->p_type ==  PT_INTERP){
            char * ld_path = kmalloc(p_header->p_memsz);
            memset(ld_path, 0, p_header->p_memsz);
            memcpy(ld_path, (elf_data+p_header->p_offset), p_header->p_filesz);

            kprintf("[ELF]  Got interpreter file path: %s\n", ld_path);
            File * ld_file = vfs_open(ld_path, 0);
            u8 * ld_data = kmalloc(ld_file->size);
            int br = vfs_read(ld_file, ld_data, 0, ld_file->size);

            if(!(br != 0 && validate_elf(ld_data))) continue;

            Elf64_Ehdr * ld_hdr = ( Elf64_Ehdr *) ld_data;
            aux.ld_entry = (LD_BASE + ld_hdr->e_entry);

            for(u64 lds=0; lds<ld_hdr->e_phnum; ++lds){
                Elf64_Phdr  * ldph = (Elf64_Phdr*) (ld_data + ld_hdr->e_phoff + (ld_hdr->e_phentsize * lds));

                if(ldph->p_type != PT_LOAD) continue;

                u64 offset = ldph->p_vaddr & (PAGE_SIZE-1);
                u64 blocks = (ldph->p_memsz/PAGE_SIZE)+2;

                void * paddr = pmm_alloc_blocks(blocks);
                void * vaddr = (void*)(LD_BASE + (ldph->p_vaddr & ~(0xfff)));

                memset(paddr, 0, ldph->p_memsz);
                memcpy(paddr+offset, (ld_data+ldph->p_offset), ldph->p_filesz);

                int page_flags = PAGE_USER | PAGE_WRITE | PAGE_PRESENT;
                vmm_map_range(vas, vaddr, paddr, blocks * PAGE_SIZE, page_flags);
                
            }
        }
		
        if(p_header->p_type != PT_LOAD) continue;

        u64 offset = p_header->p_vaddr & (PAGE_SIZE-1);
        u64 blocks = (p_header->p_memsz/PAGE_SIZE)+1;

        void * phys_addr = pmm_alloc_blocks(blocks);
        void * virt_addr = (void*)(p_header->p_vaddr-offset);

        memset(phys_addr, 0, p_header->p_memsz);
        memcpy(phys_addr+offset, (elf_data+p_header->p_offset), p_header->p_filesz);

        int page_flags = PAGE_USER | PAGE_WRITE | PAGE_PRESENT;
        for(int block=0; block<blocks; block++){
            vmm_map_page(vas, (uintptr_t)virt_addr, (uintptr_t)phys_addr, page_flags); 
            virt_addr += PAGE_SIZE; phys_addr += PAGE_SIZE;
        }
	}

    return aux;
}


#define AT_PHDR 3
#define AT_PHENT 4
#define AT_PHNUM 5
#define AT_ENTRY 9
#define AT_RANDOM 25
#define AT_EXECFN 31


ProcessControlBlock * create_elf_process(const char * path){
    File * elf_file = vfs_open(path, 0);

    u8 * elf_data = kmalloc(elf_file->size);
    int br = vfs_read(elf_file, elf_data, 0, elf_file->size);

    if(!validate_elf(elf_data)){
        return NULL;
    }

	ProcessControlBlock * proc = kmalloc(sizeof(ProcessControlBlock));

    proc->p_stack = pmm_alloc_blocks(8) + (8 * PAGE_SIZE);
    kprintf("Process stack at 0x%x\n", proc->p_stack);

    proc->cr3 = vmm_create_user_proc_pml4(proc->p_stack); 

    kprintf("Elf file size is %llu bytes\n", elf_file->size);

    void* pa_virt_start = (void*)(((u64)elf_data / PAGE_SIZE)  * PAGE_SIZE);

    void * phys_start = pmm_alloc_blocks((elf_file->size / PAGE_SIZE) + 1);
    size_t pa_size = ((elf_file->size / PAGE_SIZE) + 1)  * PAGE_SIZE; 
    vmm_map_range(proc->cr3, pa_virt_start, phys_start, pa_size,  PAGE_USER | PAGE_PRESENT | PAGE_WRITE);

    Auxval aux = load_elf_segments(proc->cr3, elf_data);

	u64 * stack = (u64 *)(proc->p_stack);

    *--stack = 0; *--stack = 0;
    *--stack = aux.entry;   *--stack = AT_ENTRY;
    *--stack = aux.phent;   *--stack = AT_PHENT;
    *--stack = aux.phnum;   *--stack = AT_PHNUM;
    *--stack = aux.phdr;    *--stack = AT_PHDR;

    *--stack = 0; // end argv
    *--stack = 0; // end envp
    *--stack = 0; // argc
    uintptr_t sa = (uintptr_t)stack;
    
    // Interrupt frame
    *--stack = 0x23; // ss
	*--stack = sa; // rsp
	*--stack = 0x202 ; // rflags
	*--stack = 0x2b; // cs
	*--stack = (u64)aux.ld_entry; // rip

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
    proc->mmap_base = 0xB000000;
    proc->next = 0;


    return proc;
}


