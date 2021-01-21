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
#include <kernel/paging.h>

//#include <kernel/vmm.h>

#include "multiboot.h"

#define INT_MAX 2147483647

#define PAGE_PRESENT    0
#define PAGE_RW         1


void Sleep(uint32_t ms);
void ATA_WAIT_INT();
uint32_t placement_address = 0;

extern void load_page_directory(uint32_t *);
extern void init_paging();
extern uint32_t ekernel;
extern uint32_t sbss;

typedef struct multiboot_mmap_entry mmap_entry_t;

static inline bool are_ints_enabled(){
    uint64_t flags;
    asm volatile("pushf\n\t" "pop %0" : "=g"(flags));
    return flags & (1 << 9);
}

int oct2bin(unsigned char *str, int size) {
    int n = 0;
    unsigned char *c = str;
    while (size-- > 0) {
        n *= 8;
        n += *c - '0';
        c++;
    }
    return n;
}

/* returns file size and pointer to file data in out */
int tar_lookup(unsigned char *archive, char *filename, char **out) {
    unsigned char *ptr = archive;
 
    while (!memcmp(ptr + 257, "ustar", 5)) {
        int filesize = oct2bin(ptr + 0x7c, 11);
        if (!memcmp(ptr, filename, strlen(filename) + 1)) {
            *out = ptr + 512;
            return filesize;
        }
        ptr += (((filesize + 511) / 512) + 1) * 512;
    }
    return 0;
}

void idpaging(uint32_t *first_pte, uint32_t from, int size) {
    from = from & 0xfffff000; // discard bits we don't want
    for(; size>0; from += 4096, size -= 4096, first_pte++){
       *first_pte = from | 1;     // mark page present.
    }
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
            printf("Entry #%d:\n", entries)	;
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

    printf("Initialized Physical Memory Manager with %ldKiB (%d blocks)\n", total_mem_size_kib,pmm_get_block_count());


    uint32_t i=0;
    while(avail_mmap[i][1] != 0){
        //printf("Addr: 0x%x Size: %ld KiB\n", avail_mmap[i][0], avail_mmap[i][1]);
        pmm_init_region(avail_mmap[i][0], avail_mmap[i][1]);
        i++;
    }
    printf("%d free physical blocks\n", pmm_get_free_block_count());


    uint64_t page_dir_ptr_tab[4] __attribute__((aligned(0x20))); //must be aligned to (at least)0x20, ...
    uint64_t page_dir[512] __attribute__((aligned(0x1000)));  // must be aligned to page boundary
    uint64_t page_tab[512] __attribute__((aligned(0x1000)));

    unsigned int address=0;
    // mapping first 2 MiB
    for(i = 0; i < 512; i++){
        page_tab[i] = address | 3; // map address and mark it present/writable
        address = address + 0x1000;
    }

    page_dir_ptr_tab[0] = (uint64_t)&page_dir | 1; // set the page directory into the PDPT and mark it present
    page_dir[0] = (uint64_t)&page_tab | 3; //set the page table into the PD and mark it present/writable



    asm volatile ("movl %%cr4, %%eax; bts $5, %%eax; movl %%eax, %%cr4" ::: "eax"); // set bit5 in CR4 to enable PAE         
    asm volatile ("movl %0, %%cr3" :: "r" (&page_dir_ptr_tab)); // load PDPT into CR3

    asm volatile ("movl %%cr0, %%eax; orl $0x80000000, %%eax; movl %%eax, %%cr0;" ::: "eax");

    //uint32_t directories[1024] __attribute__((aligned(4096))); 
    //uint32_t first_tab[1024] __attribute__((aligned(4096)));

    //// page map the first 4MiB
    //for(i=0; i<1024; i++){
    //    first_tab[i] = (i*4096) | 3;
    //}

    //// fill the rest of the memory (4MiB-4GiB)
    //directories[0]= ((uint32_t) first_tab)|3;
    //for(i=1; i<1023; i++){ // iterates directories
    //    uint32_t offset=i*4096;
    //    uint32_t table[1024] __attribute__((aligned(4096)));
    //    for(int j=0; j<1024; j++){
    //        table[j] = (offset + (j*4096))|3;
    //        phys_addr p = pmm_alloc_block();
    //        if(p) printf("Next free block at 0x%x\n", p);
    //    }
    //    directories[i]= ((uint32_t)table)|3;
    //}


    
    for(;;) asm("hlt");
    //load_page_directory((uint32_t *) &directories);
    //init_paging();

    printf("Paging is now enabled\n");
    //idpaging(&first_tab, 0, 1024*4096); // 4MiB
    //printf("Identity mapped first 4 MiB\n");


    uint16_t target[256];
    uint8_t     split[256][2];

    //if(ATA_IDENTIFY(0xA0)){ // primary drive is ready
    //    uint64_t lba=500;
    //    //while(1){
    //        read_sectors(&target, 0xA0, lba, 1);
    //        for(int i=0; i<256; i++){
    //            split[i][0]= target[i];
    //            split[i][1]= target[i] << 8;
    //            printf("%c %c ", split[i][0] , split[i][1]);
    //        }
    //        lba++;
    //    //}
    //    
    //}
    //kernel_panic("Test Panic\n");

    printf("\n");
    read_rtc();
    terminal_setcolor(0xE); // yellow
    printf(" _ _ _     _                      _              _____ _____ \n");
    printf("| | | |___| |___ ___ _____ ___   | |_ ___    ___|     |   __|\n");
    printf("| | | | -_| |  _| . |     | -_|  |  _| . |  |- _|  |  |__   |\n");
    printf("|_____|___|_|___|___|_|_|_|___|  |_| |___|  |___|_____|_____|\n");
    terminal_setcolor(0xF); // white

    

    //for(;;) asm("hlt");
    asm("hlt");

}
