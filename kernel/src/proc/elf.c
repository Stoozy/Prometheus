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

#define LINKER_BASE 0xc000000

static void load_segments(PageTable * vas, u8 * elf){
    Elf64_Ehdr * elf64 = (Elf64_Ehdr *) elf;

    for(u64 segment=0; segment<elf64->e_phnum; ++segment){
        Elf64_Phdr * p_header = (Elf64_Phdr *) 
            (elf + elf64->e_phoff + (elf64->e_phentsize * segment));

        if(p_header->p_type == PT_INTERP){
            char * ld_path = kmalloc(p_header->p_memsz);
            memcpy(ld_path, elf + p_header->p_offset, p_header->p_memsz);
            kprintf("[ELF]  Got linker path: %s ", ld_path);
            FILE * f = vfs_open((const char *)ld_path, 0);
            char * ld_buf = kmalloc(f->size);
            int br = vfs_read(f, f->size, (u8*)ld_buf);
            if(br != 0){
                kprintf("[ELF]  Read %s; Contents: %s\n", ld_path, ld_buf);
                load_segments(vas, (u8*)ld_buf);
            }


        }

        if(p_header->p_type == PT_LOAD){
            /* found loadable segment */

            kprintf("[ELF] Loading segments for ld.so\n");
            int flags = PAGE_USER | PAGE_READ_WRITE | PAGE_PRESENT;

            u64 blocks = ((p_header->p_vaddr+p_header->p_filesz)/PAGE_SIZE - p_header->p_vaddr/PAGE_SIZE) +1;
            void * phys_addr = pmm_alloc_blocks(blocks);

            kprintf("[ELF]  Found loadable segment at offset 0x%x\n", p_header->p_offset);
            memset(phys_addr, 0, p_header->p_filesz);
            memcpy(phys_addr, 
                    (void*)elf+(p_header->p_offset), p_header->p_memsz);

            // now map those pages 
            void* virt_addr =  (void*)p_header->p_vaddr;

            for(u64 page = 0; page<blocks; ++page){
                kprintf("[ELF]  Mapping 0x%x virt to 0x%x phys\n",
                        LINKER_BASE + virt_addr, phys_addr+(page*PAGE_SIZE));

              vmm_map(vas, LINKER_BASE + virt_addr, phys_addr+(page*PAGE_SIZE), flags);
              virt_addr += PAGE_SIZE;
          }
      }
    }
}

u8 load_elf_64( u8 * elf){
    Elf64_Ehdr * elf64 = (Elf64_Ehdr *) elf;

    ProcessControlBlock * proc = create_process((void*)elf64->e_entry);

    for(u64 segment=0; segment<elf64->e_phnum; ++segment){
        Elf64_Phdr * p_header = (Elf64_Phdr *) 
            (elf + elf64->e_phoff + (elf64->e_phentsize * segment));

        if(p_header->p_type == PT_INTERP){
            char * ld_path = kmalloc(p_header->p_memsz);
            u8 * ld_buf = kmalloc(p_header->p_filesz);
            memcpy(ld_path, elf + p_header->p_offset, p_header->p_memsz);
            kprintf("[ELF]  Got linker path: %s ", ld_path);
            FILE * f = vfs_open((const char *)ld_path, 0);
            int br = vfs_read(f, f->size, ld_buf);
            if(br != 0){
                kprintf("[ELF]  Read %s; Contents: %s\n", ld_path, ld_buf);
                Elf64_Ehdr * interpElf = (Elf64_Ehdr*) ld_buf;
                load_segments(proc->cr3, ld_buf);

/* from drip os */
#define AT_PHDR 3
#define AT_PHENT 4
#define AT_PHNUM 5
#define AT_ENTRY 9
#define AT_RANDOM 25
#define AT_EXECFN 31


                u64 * stack = (u64*)proc->p_stack;
                stack[12] =  LINKER_BASE + interpElf->e_entry;
                *--stack = 0;
                *--stack = 0;
                *--stack = elf64->e_entry;
                *--stack = AT_ENTRY;
                *--stack = (u64)p_header;
                *--stack = AT_PHDR;
                *--stack = elf64->e_phentsize;
                *--stack = AT_PHENT;
                *--stack = elf64->e_phnum;
                *--stack = AT_PHNUM;

                //*--stack = 0; // argc
                //*--stack = 0; // argv
                //*--stack = elf64->e_entry; 
                //*--stack = 9;
                //*--stack = (u64)p_header; 
                //*--stack = 3; 
                //*--stack = (u64)elf64->e_phentsize; 
                //*--stack = 4; 
                //*--stack = (u64)elf64->e_phnum; 
                //*--stack = 5; 

                //*--stack = 0x23; // ss
                //*--stack = (u64)proc->p_stack; // rsp
                //*--stack = 0x202 ; // rflags
                //*--stack = 0x2b; // cs
                //*--stack = (u64)interpElf->e_entry; // rip

                //*--stack = 0; // r8
                //*--stack = 0;
                //*--stack = 5; 
                //*--stack = elf64->e_phnum; // ...
                //*--stack = 4; 
                //*--stack = elf64->e_phentsize;
                //*--stack = 3;
                //*--stack = p_header; // r15


                //*--stack = 9; // rbp

                //*--stack = elf64->e_entry; // rdx
                //*--stack = 0; // rsi
                //*--stack = 0; // rdi

                //proc->p_stack = stack;
            }


        }

        if(p_header->p_type == PT_LOAD){
            /* found loadable segment */
            kprintf("[ELF] Loading segment for hello\n");
            int flags = PAGE_USER | PAGE_READ_WRITE | PAGE_PRESENT;

            u64 blocks = ((p_header->p_vaddr+p_header->p_filesz)/PAGE_SIZE - p_header->p_vaddr/PAGE_SIZE) +1;
            void * phys_addr = pmm_alloc_blocks(blocks);

            kprintf("[ELF]  Found loadable segment at offset 0x%x\n", p_header->p_offset);
            memset(phys_addr, 0, p_header->p_memsz);
            memcpy(phys_addr, 
                    (void*)elf+(p_header->p_offset), p_header->p_memsz);

            // now map those pages 
            void* virt_addr =  (void*)p_header->p_vaddr;

            for(u64 page = 0; page<blocks; ++page){
                kprintf("[ELF]  Mapping 0x%x virt to 0x%x phys\n",
                        virt_addr, phys_addr+(page*PAGE_SIZE));
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


