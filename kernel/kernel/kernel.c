#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/io.h>
#include <kernel/rtc.h>
#include <kernel/util.h>

#include "multiboot.h"
void Sleep(uint32_t ms);

typedef struct multiboot_memory_map {
	uint32_t size;
	uint32_t base_addr_low,base_addr_high;
// You can also use: unsigned long long int base_addr; if supported.
	uint32_t length_low,length_high;
// You can also use: unsigned long long int length; if supported.
	uint32_t type;
} mb_mmap_t;

typedef mb_mmap_t mmap_entry_t;

extern void do_e820();
extern void get_mm();

static inline bool are_ints_enabled(){
    uint64_t flags;
    asm volatile("pushf\n\t" "pop %0" : "=g"(flags));
    return flags & (1 << 9);
}


void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
	terminal_initialize();

    init_gdt();
    init_idt();

	printf("%x\n", mbd);
	printf("magic is: %x\n", magic);
    bool interrupt_status  = are_ints_enabled();

    if(interrupt_status) printf("Interrupt requests are currently enabled\n");
    else printf("Interrupt requests are currently disabled\n");

	//printf("MMAP_ADDR: %x \nMEM_LOW:%d\nMEM_UPPER:%d\n", mbd->mmap_addr, mbd->mem_lower, mbd->mem_upper);
    
	int entries = 0;
	mmap_entry_t* entry = mbd->mmap_addr;
	while(entry < (mbd->mmap_addr + mbd->mmap_length)) {
		// do something with the entry
		entry = (mmap_entry_t*) ((unsigned int) entry + entry->size + sizeof(entry->size));
		if(entry->type == MULTIBOOT_MEMORY_AVAILABLE){
			uint64_t total_addr = entry->base_addr_low | entry->base_addr_high << 32;
			uint64_t total_len = entry->length_low| entry->length_high << 32;
			printf("Entry #%d:\n", entries)	;
			printf("    Size: %ld\n", entry->size);
			printf("    Address: %llx\n", total_addr);
			printf("    Length: %llu\n", total_len);
		}
		entries++;
	}


    terminal_setcolor(0xE); // yellow
    printf("%s"," _ _ _     _                      _              _____ _____ \n");
    printf("%s","| | | |___| |___ ___ _____ ___   | |_ ___    ___|     |   __|\n");
    printf("%s","| | | | -_| |  _| . |     | -_|  |  _| . |  |- _|  |  |__   |\n");
    printf("%s","|_____|___|_|___|___|_|_|_|___|  |_| |___|  |___|_____|_____|\n");
    terminal_setcolor(0xF); // white


    outb(0x70, 0x30);
    uint8_t lowmem = inb(0x71);
    outb(0x70, 0x31);
    uint8_t highmem = inb(0x71);
 
    uint16_t total = lowmem | highmem << 8;
	//printf("CMOS Memory Map: %d\n", total);

	read_rtc();
	
    for(;;){
		asm("hlt");
    }
}
