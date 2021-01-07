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
#include <kernel/paging.h>

#include "multiboot.h"

#define INT_MAX 2147483647



void Sleep(uint32_t ms);
void ATA_WAIT_INT();
uint32_t placement_address = 0;

extern void load_page_directory(uint32_t *);
extern void init_paging();

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

void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
	terminal_initialize();

    init_gdt();
    init_idt();


	printf("Kernel main loaded at: %x\n", kernel_main);

	printf("magic is: %x\n", magic);

    bool interrupt_status  = are_ints_enabled();
		

    //uint16_t target[2560];
    //uint8_t drive = 0xA0; // master
    //bool is_valid_disk = ATA_IDENTIFY(drive);
    //      

    //outb(0x3F6, 4);
    //asm("nop");
    //outb(0x3F6, 0);


    //if(is_valid_disk){
    //    //for(int k=0; k<INT_MAX; k++){
    //      read_sectors(target, drive, 2, 10);
    //      for(int i=0; i<2560; i++) {
    //            printf("%c %c ", target[i], target[i] >> 8);
    //            Sleep(54);
    //      }
    //      printf("\n");

    //    //}
    //}

    if(interrupt_status) printf("Interrupt requests are currently enabled\n");
    else printf("Interrupt requests are currently disabled\n");

	printf("MMAP_ADDR: %x \nMEM_LOW:%d\nMEM_UPPER:%d\n", mbd->mmap_addr, mbd->mem_lower, mbd->mem_upper);
    
	int entries = 0;
	mmap_entry_t* entry = mbd->mmap_addr;
    uint32_t mem_end_page=0;	
	while(entry < (mbd->mmap_addr + mbd->mmap_length)) {
		entry = (mmap_entry_t*) ((unsigned int) entry + entry->size + sizeof(entry->size));
		if(entry->type == MULTIBOOT_MEMORY_AVAILABLE){
			//uint64_t total_addr = entry->base_addr_low | entry->base_addr_high << 32;
			//uint64_t total_len = entry->length_low| entry->length_high << 32;
			printf("Entry #%d:\n", entries)	;
			printf("    Size: %d\n", entry->size); 
			printf("    Address: %llxB\n", entry->addr);
			printf("    Length: %lluMiB\n", (entry->len/(1024*1024)));
		}
		entries++;
	}

	uint64_t page_dir_ptr_tab[4] __attribute__((aligned(32)));

	// pointers to tables
	uint64_t page_dir[512] __attribute__((aligned(4096)));
	// table
	uint64_t page_tab[512] __attribute__((aligned(4096)));

	// set the page directory into the PDPT and mark it present
	page_dir_ptr_tab[0] = (uint64_t)&page_dir | 1;
	//set the page table into the PD and mark it present/writable
	page_dir[0] = (uint64_t)&page_tab | 3;
	
    uint32_t i, address = 0;
    for(i=0; i<512; i++){
		// map address and mark it present/writable
		page_tab[i] = address | 3; 
		address += 4096; 
    }
	/* 
	*	set bit5 in CR4 to enable PAE
	* 	load PDPT into CR3
	*/ 	
	asm volatile ("movl %%cr4, %%eax; bts $5, %%eax; movl %%eax, %%cr4" ::: "eax"); 		 
	asm volatile ("movl %0, %%cr3" :: "r" (&page_dir_ptr_tab)); 

	// activate paging
	asm volatile ("movl %%cr0, %%eax; orl $0x80000000, %%eax; movl %%eax, %%cr0;" ::: "eax");

	// get the page directory (you should 'and' the flags away)
	uint64_t * pd = (uint64_t*)page_dir_ptr_tab[3]; 
	pd[511] = (uint64_t)pd; // map pd to itself
	pd[510] = page_dir_ptr_tab[2]; // map pd3 to it
	pd[509] = page_dir_ptr_tab[1]; // map pd2 to it
	pd[508] = page_dir_ptr_tab[0]; // map pd1 to it
	pd[507] = (uint64_t)&page_dir_ptr_tab; //	map the PDPT to the directory

	// contains physical addresses
	//uint32_t j; 
	////we will fill all 1024 entries in the table, mapping 4 megabytes
	//for(j = 0; j < 512; j++){
	//	// As the address js page aligned, jt wjll always leave 12 bjts zeroed.
	//	// Those bjts are used by the attrjbutes ;)
	//	// attrjbutes: supervjsor level, read/wrjte, present.
	//	first_page_table[j] = (j * 4096) | 3;
	//}

	//page_directory[0] = ((uint32_t) first_page_table) | 3;
	//load_page_directory(page_directory);
	//init_paging();
	printf("Paging is enabled\n");

	//idpaging(&first_page_table, 0x0, 2048); // 8MiB
	//printf("Identitiy mapped the first 8 MiB\n");

	read_rtc();
    terminal_setcolor(0xE); // yellow
    printf("%s"," _ _ _     _                      _              _____ _____ \n");
    printf("%s","| | | |___| |___ ___ _____ ___   | |_ ___    ___|     |   __|\n");
    printf("%s","| | | | -_| |  _| . |     | -_|  |  _| . |  |- _|  |  |__   |\n");
    printf("%s","|_____|___|_|___|___|_|_|_|___|  |_| |___|  |___|_____|_____|\n");
    terminal_setcolor(0xF); // white



	
    for(;;){
		asm("hlt");
    }
}
