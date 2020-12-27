#include <stdbool.h>
#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>

static inline bool are_ints_enabled(){
    uint64_t flags;
    asm volatile("pushf\n\t" "pop %0" : "=g"(flags));
    return flags & (1 << 9);
}

void kernel_main(void) {
	terminal_initialize();

    init_gdt();
    init_idt();


    bool interrupt_status  = are_ints_enabled();
   
    if(interrupt_status) printf("[KERNEL] Interrupt requests are currently enabled\n");
    else printf("[KERNEL] Interrupt requests are currently disabled\n");

    terminal_setcolor(0xE); // yellow
    printf("%s"," _ _ _     _                      _              _____ _____ \n");
    printf("%s","| | | |___| |___ ___ _____ ___   | |_ ___    ___|     |   __|\n");
    printf("%s","| | | | -_| |  _| . |     | -_|  |  _| . |  |- _|  |  |__   |\n");
    printf("%s","|_____|___|_|___|___|_|_|_|___|  |_| |___|  |___|_____|_____|\n");
    terminal_setcolor(0xF); // white

    printf("Positive int test: %d\n", 34234);
    printf("Negative int test: %d\n", -94823);

    printf("Hexadecimal test: %x\n", 0x05AB6);

    for(;;){
        asm ("hlt");
    }
}
