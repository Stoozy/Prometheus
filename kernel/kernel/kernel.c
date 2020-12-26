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

    printf("Welcome to zOS!\n");
    printf("%d", -23294); 
    printf("\n");
    printf("%d", 34934); 

    for(;;){
        asm ("hlt");
    }
}
