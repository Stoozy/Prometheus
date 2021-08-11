
#include "typedefs.h"

//#include <stdint.h>
#include <stddef.h>
#include "stivale.h"
#include "cpu/io.h"
#include "cpu/idt.h"

#include "memory/pmm.h"
#include "memory/vmm.h"

#include "drivers/serial.h"
#include "drivers/gui.h"
#include "drivers/pit.h"

#include "util.h"
#include "kprintf.h"


extern u64 k_start;
extern u64 k_end;

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


void _start(struct stivale_struct * boot_info) {
    serial_init(); /* init debugging */

    pmm_init(boot_info); /* reads memory map and initializes memory manager */
    
    u64 k_size = ((u64)&k_end - (u64)&k_start);

    kprintf("[_start]   Kernel starts at 0x%x\n", &k_start);
    kprintf("[_start]   Kernel ends at 0x%x\n", &k_end);
    kprintf("[_start]   Kernel size is %lu bytes\n", k_size);

    u64 fb_size = boot_info->framebuffer_bpp * 
        boot_info->framebuffer_height * boot_info->framebuffer_width;

    /* lock kernel blocks and frame buffer blocks */
    //pmm_mark_region_used((void*)&k_start, k_size + 0x1000);
    pmm_mark_region_used(&k_start, &k_end);
    pmm_mark_region_used(boot_info->framebuffer_addr, 
                            boot_info->framebuffer_addr+fb_size);

    vmm_init();

    vmm_map((void*)0xffffffff80000000, boot_info->framebuffer_addr);
    u32 * buf = (u32 *) 0xffffffff80000000;

    for(u32 i=0; i<4096/4; ++i){
        buf[i] = 0xffffff;
    }

    kprintf("Set some pixels");

    cli();
    while(1);

    /*pit_init(1000);
    idt_init();
    */
 
    /* kprintf("------------------Framebuffer Information--------------\n");
    kprintf("[KMAIN]    Framebuffer height : %d\n", stivale_struct->framebuffer_height);
    kprintf("[KMAIN]    Framebuffer width : %d\n", stivale_struct->framebuffer_width);
    kprintf("[KMAIN]    Framebuffer addr : 0x%x\n", stivale_struct->framebuffer_addr);
    kprintf("[KMAIN]    Framebuffer bpp : %d\n", stivale_struct->framebuffer_bpp);
    */


    // We're done, just hang...
    for (;;) { asm ("hlt"); }
    
}

