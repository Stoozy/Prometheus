#define SSFN_CONSOLEBITMAP_TRUECOLOR
#define NULL (void*)0

#include "typedefs.h"
#include "stivale.h"
#include "cpu/io.h"
#include "cpu/idt.h"
#include "misc/ssfn.h"

#include "memory/pmm.h"
#include "memory/vmm.h"

#include "drivers/serial.h"
#include "drivers/video.h"
#include "drivers/pit.h"

#include "process/elf.h"

#include "util.h"
#include "kmalloc.h"
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
    serial_init();                      /* init debugging */

    struct stivale_module * module = (struct stivale_module *)boot_info->modules;
    //ssfn_src = (ssfn_font_t*)module->begin;                    /* the bitmap font to use */
    
    kprintf("[_start]   Kernel physical start address : 0x%x\n", vmm_virt_to_phys((void*)&k_start));
    kprintf("[_start]   Kernel physical end address : 0x%x\n", vmm_virt_to_phys((void*)&k_end));

    /* allocate 5 MiB for kernel memory */
    kprintf("[_start]   Kernel starts at 0x%x\n", &k_start);
    kprintf("[_start]   Kernel ends at 0x%x\n", &k_end);

    u64 module_size =  module->end - module->begin;
    kprintf("[_start]   Modules begin at 0x%x\n", module->begin);
    kprintf("[_start]   Module name : %s\n", module->string);
    kprintf("[_start]   Module size: %lu bytes\n", module_size);
    kprintf("\n");

    

    //kprintf(" %c %c %c %c", ssfn_src->magic[0], ssfn_src->magic[1], ssfn_src->magic[2], ssfn_src->magic[3]);

    //ssfn_dst.ptr = (u8*)boot_info->framebuffer_addr;            /* address of the linear frame buffer */
    //ssfn_dst.w = boot_info->framebuffer_width;                  /* width */
    //ssfn_dst.h = boot_info->framebuffer_height;                 /* height */
    //ssfn_dst.p = boot_info->framebuffer_width*
    //    (boot_info->framebuffer_bpp/8);                         /* bytes per line */
    //ssfn_dst.x = ssfn_dst.y = 0;                                /* pen position */
    //ssfn_dst.fg = 0xFFFFFF;                                     /* foreground color */


    pmm_init(boot_info);                /* reads memory map and initializes memory manager */
    
    /* TODO: move this to the pmm_init func */
    u64 fb_size = (boot_info->framebuffer_bpp/8) * 
        boot_info->framebuffer_height * boot_info->framebuffer_width;

    pmm_mark_region_used((void*)boot_info->framebuffer_addr, 
                            (void*)boot_info->framebuffer_addr+fb_size);


    u64 k_size = ((u64)&k_end - (u64)&k_start);
    kprintf("[_start]   Kernel size is %d bytes (0x%x)\n", k_size, k_size);

    vmm_init(boot_info);


    screen_init(boot_info);
    kprintf("[_start]   Kprintf test\n");
    pit_init(1000);
    idt_init();


    kprintf("[_start]   Size of 64-bit elf header %d bytes\n", sizeof(ElfHeader64));

    // We're done, just hang...
    for (;;) { asm ("hlt"); }
    
}

