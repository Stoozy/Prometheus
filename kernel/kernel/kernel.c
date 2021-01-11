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
#include <kernel/pmm.h>

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


	printf("Kernel main loaded at: 0x%x\n", kernel_main);

	printf("magic is: %x\n", magic);

    
    bool interrupt_status  = are_ints_enabled();
    if(interrupt_status) printf("Interrupt requests are currently enabled\n");
    else printf("Interrupt requests are currently disabled\n");

	printf("MMAP_ADDR: 0x%x \nMEM_LOW:%d\nMEM_UPPER:%d\n", mbd->mmap_addr, mbd->mem_lower, mbd->mem_upper);

    uint32_t avail_mmap[3][2]= {0}; // first col: addr, second col: size
    
	int entries = 0, avail_entries=0;
	mmap_entry_t* entry = mbd->mmap_addr;
    uint64_t total_mem_size_kib =0;
    uint32_t mem_end_page=0;	
	while(entry < (mbd->mmap_addr + mbd->mmap_length)) {
		entry = (mmap_entry_t*) ((unsigned int) entry + entry->size + sizeof(entry->size));
			//uint64_t total_addr = entry->base_addr_low | entry->base_addr_high << 32;
			//uint64_t total_len = entry->length_low| entry->length_high << 32;
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

    uint32_t * pmm_bitmap=0;
    pmm_init(total_mem_size_kib, pmm_bitmap);
    printf("Initialized Physical Memory Manager with %ldKiB (%d blocks)\n", total_mem_size_kib, pmm_get_block_count());


	uint32_t i=0;
    while(avail_mmap[i][1] != 0){
        pmm_init_region(avail_mmap[i][0], avail_mmap[i][1]);
        printf("Initalized physical memory region starting at 0x%x of size: %lldMiB\n",avail_mmap[i][0], (avail_mmap[i][1]/(1024*1024)));
        i++;
    }
    

    uint32_t page_dir[1024] __attribute__((aligned(4096)));
    uint32_t page_tab[1024] __attribute__((aligned(4096)));
    uint32_t second_page_tab[1024] __attribute__((aligned(4096)));

	// holds the physical address where we want to start mapping these pages to.
	// in this case, we want to map these pages to the very beginning of memory.
	 
	//we will fill all 1024 entries in the table, mapping 4 megabytes
	for(i = 0; i < 1024; i++){
		// As the address is page aligned, it will always leave 12 bits zeroed.
		// Those bits are used by the attributes ;)
		page_tab[i] = (i * 0x1000) | 3; // attributes: supervisor level, read/write, present.
	}

    // mapping another 4 MiB
    for(i=0; i<1024; i++){
        second_page_tab[i] = (i*0x1000) | 3;
    }

    // attributes: supervisor level, read/write, present
    page_dir[0] = ((unsigned int)page_tab) | 3;
    //page_dir[1] = ((unsigned int)second_page_tab) | 3;

	load_page_directory((uint32_t *)&page_dir);
	init_paging();

	printf("Paging has been setup. \n");


	//idpaging(&page_tab, 0x0, 1048576); // 1MiB
    printf("Identity mapped first MiB\n");
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
