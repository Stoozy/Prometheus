
#include "typedefs.h"

//#include <stdint.h>
#include <stddef.h>
#include "stivale.h"
#include <cpu/io.h>
#include <cpu/idt.h>

#include <memory/pmm.h>

#include <drivers/serial.h>
#include <drivers/gui.h>
#include <drivers/pit.h>

#include <kprintf.h>


// We need to tell the stivale bootloader where we want our stack to be.
// We are going to allocate our stack as an uninitialised array in .bss.
static u8 stack[4096];

// The stivale specification says we need to define a "header structure".
// This structure needs to reside in the .stivalehdr ELF section in order
// for the bootloader to find it. We use this __attribute__ directive to
// tell the compiler to put the following structure in said section.
__attribute__((section(".stivalehdr"), used))
static struct stivale_header stivale_hdr = {
    .stack = (uintptr_t)stack + sizeof(stack),
    /* feature flags */
    .flags = (1 << 0) | (1 << 3),
    // We set all the framebuffer specifics to 0 as we want the bootloader
    // to pick the best it can.
    .framebuffer_width  = 0,
    .framebuffer_height = 0,
    .framebuffer_bpp    = 0,
    .entry_point = 0
};


// The following will be our kernel's entry point.
void _start(struct stivale_struct * stivale_struct) {
    // Let's get the address of the framebuffer.
    u32 * fb_addr = (u32 *)stivale_struct->framebuffer_addr;
    // Let's try to paint a few pixels white in the top left, so we know
    // that we booted correctly.
    serial_init();

    struct stivale_mmap_entry * mmap_entries = 
        (struct stivale_mmap_entry * ) stivale_struct->memory_map_addr;

    pmm_init(); /* just sets fields */

    kprintf("------------------Memory Information-------------------\n");

    for(int i=0; i<stivale_struct->memory_map_entries;++i){
        if(mmap_entries[i].type == STIVALE_MMAP_USABLE){
            pmm_init_region((void*) mmap_entries[i].base, mmap_entries[i].length);
            kprintf("[KMAIN]    Base 0x%x\n", mmap_entries[i].base);
            kprintf("[KMAIN]    Size %llu bytes\n", mmap_entries[i].length);
            kprintf("[KMAIN]    Type %d\n", mmap_entries[i].type);
        }
    }

    pmm_dump();


    kprintf("------------------Framebuffer Information--------------\n");
    kprintf("[KMAIN]    Framebuffer height : %d\n", stivale_struct->framebuffer_height);
    kprintf("[KMAIN]    Framebuffer width : %d\n", stivale_struct->framebuffer_width);
    kprintf("[KMAIN]    Framebuffer addr : 0x%x\n", stivale_struct->framebuffer_addr);
    kprintf("[KMAIN]    Framebuffer bpp : %d\n", stivale_struct->framebuffer_bpp);


    cli();
    while(1);

    pit_init(1000);
    idt_init();

    kprintf("sleeping for 5 seconds\n");
    sleep(5000);
    kprintf("Done sleeping for 5 seconds\n");

    // We're done, just hang...
    for (;;) {
        asm ("hlt");
    }
    
}
