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

#include "multiboot.h"

void Sleep(uint32_t ms);
void ATA_WAIT_INT();

//typedef mb_mmap_t mmap_entry_t;
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


void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
	terminal_initialize();

    init_gdt();
    init_idt();

	printf("Kernel main loaded at: %x\n", kernel_main);

	printf("magic is: %x\n", magic);
    bool interrupt_status  = are_ints_enabled();

    

    uint8_t drive = 0xA0; // master
    bool is_valid_disk = ATA_IDENTIFY(drive);

    if(is_valid_disk){
        uint16_t target[256];
        read_sectors(target, 1, 0, 1);
        printf("Reading from disk: \n");
        for(int i=0; i<256; i++){
            printf("%d ", target[i]);
        }
        printf("\n");

        uint16_t zeroes[256] = {0};

        write_sectors(zeroes, 1, 0);
        read_sectors(target, 1, 0, 1);

        for(int i=0; i<256; i++){
            printf("%d ", target[i]);
        }
        printf("\n");
    }else printf("Invalid disk\n");

    //if(interrupt_status) printf("Interrupt requests are currently enabled\n");
    //else printf("Interrupt requests are currently disabled\n");

	//printf("MMAP_ADDR: %x \nMEM_LOW:%d\nMEM_UPPER:%d\n", mbd->mmap_addr, mbd->mem_lower, mbd->mem_upper);
    //
	//int entries = 0;
	//mmap_entry_t* entry = mbd->mmap_addr;
	//while(entry < (mbd->mmap_addr + mbd->mmap_length)) {
	//	entry = (mmap_entry_t*) ((unsigned int) entry + entry->size + sizeof(entry->size));
	//	if(entry->type == MULTIBOOT_MEMORY_AVAILABLE){
	//		//uint64_t total_addr = entry->base_addr_low | entry->base_addr_high << 32;
	//		//uint64_t total_len = entry->length_low| entry->length_high << 32;
	//		printf("Entry #%d:\n", entries)	;
	//		printf("    Size: %d\n", entry->size); 
	//		printf("    Address: %llx\n", entry->addr);
	//		printf("    Length: %llu\n", entry->len);
	//	}
	//	entries++;
	//}


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
