#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/io.h>
#include <kernel/rtc.h>
#include <kernel/util.h>

void Sleep(uint32_t ms);

static inline bool are_ints_enabled(){
    uint64_t flags;
    asm volatile("pushf\n\t" "pop %0" : "=g"(flags));
    return flags & (1 << 9);
}

void kernel_main(void) {
    uint8_t time[128];
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

   

	//for(int i=0; i<128; i++){
    //    printf("%d ", time[i]);
    //}

    for(;;){
		read_rtc();
		asm("hlt");
    }
}
