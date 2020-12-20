#include <stdio.h>
#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>

void kernel_main(void) {
	terminal_initialize();

    init_gdt();
    init_idt();
    printf("Welcome to zOS!\n");
    //for(int i=0; i<10; i++){
    //    printf("%d", i);
    //}
}
