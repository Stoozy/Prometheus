/*Copyright 2021 Stoozy 

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/io.h>
#include <kernel/rtc.h>
#include <kernel/util.h>
#include <kernel/ata.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>
#include <kernel/paging.h>

//#include <kernel/vmm.h>

#include "multiboot.h"

#define INT_MAX 2147483647

#define PAGE_PRESENT    0
#define PAGE_RW         1


void Sleep(uint32_t ms);
void ATA_WAIT_INT();


typedef struct multiboot_mmap_entry mmap_entry_t;

static inline bool are_ints_enabled(){
    uint64_t flags;
    asm volatile("pushf\n\t" "pop %0" : "=g"(flags));
    return flags & (1 << 9);
}

void hang(){
    for(;;) asm("hlt");
}

void kernel_panic(const char * reason){
    asm("cli");
    terminal_initialize();
    terminal_setcolor(0x1F);
    terminal_panic();


    printf("\n\n\n\n");

    printf("                            ");

    terminal_setcolor(0x71);
    printf(" zos bruh moment \n");
    printf("\n\n\n\n");

    terminal_setcolor(0x1F);
    printf("            ");
    printf("zos crashed again. I am the blue screen of death. No one \n");

    printf("            ");
    printf("hears your screams.\n\n");

    printf("            ");
    printf("What happened: ");
    printf(reason);

    // hang
    for(;;);
}

void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
    terminal_initialize();
    printf("Initialized terminal\n");
    init_gdt();

    init_idt();

    printf("Kernel main loaded at: 0x%x\n", kernel_main);

    printf("magic is: %x\n", magic);

    bool interrupt_status  = are_ints_enabled();
    if(interrupt_status) printf("Interrupt requests are currently enabled\n");
    else printf("Interrupt requests are currently disabled\n");

    printf("MMAP_ADDR: 0x%x \nMEM_LOW:%d\nMEM_UPPER:%d\n", mbd->mmap_addr, mbd->mem_lower, mbd->mem_upper);

    uint32_t avail_mmap[3][2]= {0}; // first col: addr, second col: size
    uint64_t ll = 32768 *1024;

    int entries = 0, avail_entries=0;
    mmap_entry_t* entry = mbd->mmap_addr;
    uint64_t total_mem_size_kib =0;
    uint32_t mem_end_page=0;
    while(entry < (mbd->mmap_addr + mbd->mmap_length)) {
        entry = (mmap_entry_t*) ((unsigned int) entry + entry->size + sizeof(entry->size));
        if(entry->type == MULTIBOOT_MEMORY_AVAILABLE){
            printf("    Size: %d\n", entry->size); 
            printf("    Address: 0x%x\n", entry->addr);
            printf("    Length: %lluMiB\n", (entry->len/(1024*1024)));
            printf("    Type: %d\n", entry->type);
            total_mem_size_kib += (entry->len)/1024;
            // fill available entry
            avail_mmap[avail_entries][0] = entry->addr;
            avail_mmap[avail_entries][1] = entry->len;
            avail_entries++;
        }
        entries++;
    }

    printf("Total available memory: %d MiB\n", (total_mem_size_kib/(1024))); // converto MiB

    //printf("End of kernel is at: 0x%x\n",ekernel );
    pmm_init(total_mem_size_kib);

    printf("Initialized Physical Memory Manager with %ldKiB (%d blocks)\n", total_mem_size_kib, pmm_get_block_count());


    uint32_t i=0;
    while(avail_mmap[i][1] != 0){
        printf("Addr: 0x%x Size: %lu KiB\n", avail_mmap[i][0], avail_mmap[i][1]);
        pmm_init_region(avail_mmap[i][0], avail_mmap[i][1]);
        i++;
    }
    
    printf("%d free physical blocks\n", pmm_get_free_block_count());


    uint32_t addr = pmm_alloc_block();
    if(!addr)  printf("allocation failed\n");
    else{
        printf("Found a block at: 0x%x\n", addr);
        char * ptr = (char *)addr;
        ptr[0] = 'H';
        ptr[1] = 'e';
        ptr[2] = 'l';
        ptr[3] = 'l';
        ptr[4] = 'o';
        ptr[5] = ' ';
        ptr[6] = 'p';
        ptr[7] = 'm';
        ptr[8] = 'm';
        ptr[9] = '\0';
        printf(ptr);
    }



    printf("\n");
    read_rtc();

    terminal_setcolor(0xE); // yellow
    printf(" _ _ _     _                      _              _____ _____ \n");
    printf("| | | |___| |___ ___ _____ ___   | |_ ___    ___|     |   __|\n");
    printf("| | | | -_| |  _| . |     | -_|  |  _| . |  |- _|  |  |__   |\n");
    printf("|_____|___|_|___|___|_|_|_|___|  |_| |___|  |___|_____|_____|\n");
    terminal_setcolor(0xF); // white

    vmm_init();

    for(;;) asm("hlt");
}
