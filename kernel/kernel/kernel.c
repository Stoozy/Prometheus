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


    bool interrupt_status  = are_ints_enabled();
   
    if(interrupt_status) printf("Interrupt requests are currently enabled\n");
    else printf("Interrupt requests are currently disabled\n");

    terminal_setcolor(0xE); // yellow
    printf("%s"," _ _ _     _                      _              _____ _____ \n");
    printf("%s","| | | |___| |___ ___ _____ ___   | |_ ___    ___|     |   __|\n");
    printf("%s","| | | | -_| |  _| . |     | -_|  |  _| . |  |- _|  |  |__   |\n");
    printf("%s","|_____|___|_|___|___|_|_|_|___|  |_| |___|  |___|_____|_____|\n");
    terminal_setcolor(0xF); // white

	printf("MM_ADDR: %x \n MEM_LOW:%d\n MEM_UPPER:%d\n", mbd->mmap_addr, mbd->mem_lower, mbd->mem_upper);
    //outb(0x70, 0x30);
    //uint8_t lowmem = inb(0x71);
    //outb(0x70, 0x31);
    //uint8_t highmem = inb(0x71);
 
    //uint16_t total = lowmem | highmem << 8;
	//printf("CMOS Memory Map: %d\n", total);

	read_rtc();
	
    for(;;){
		asm("hlt");
    }
}
