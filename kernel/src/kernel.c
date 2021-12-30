#define SSFN_CONSOLEBITMAP_TRUECOLOR
#define NULL (void*)0

#include "typedefs.h"
#include "stivale.h"
#include "cpu/io.h"
#include "cpu/idt.h"
#include "cpu/gdt.h"
#include "misc/ssfn.h"

#include "memory/pmm.h"
#include "memory/vmm.h"

#include "drivers/serial.h"
#include "drivers/video.h"
#include "drivers/pit.h"

#include "util.h"
#include "kmalloc.h"
#include "kprintf.h"

#include "proc/proc.h"
#include "proc/elf.h"

#include "fs/tmpfs.h"

extern u64 k_start;
extern u64 k_end;

extern void load_pagedir(PageTable *);
extern void invalidate_tlb();
extern void gdt_init();
// We need to tell the stivale bootloader where we want our stack to be.
// We are going to allocate our stack as an uninitialised array in .bss.
volatile u8 stack[4096];
volatile u8 user_stack[4096];

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

extern void enable_sce();
extern void to_userspace(void* entry, void* stack);


__attribute__ ((aligned(0x1000))) void userspace_func(){
    for(;;) kprintf("Bonjour\n");
}

void hang(){
    for (;;) { asm ("hlt"); }
}
void _start(struct stivale_struct * boot_info) {

    gdt_init();
    serial_init();                      /* init debugging */

    struct stivale_module * module = (struct stivale_module *)boot_info->modules;
    //ssfn_src = (ssfn_font_t*)module->begin;                    /* the bitmap font to use */
    
    kprintf("[_start]   Kernel starts at 0x%x\n", &k_start);
    kprintf("[_start]   Kernel ends at 0x%x\n", &k_end);

    u64 module_size =  module->end - module->begin;
    kprintf("[_start]   Modules begin at 0x%x\n", module->begin);
    kprintf("[_start]   Module name : %s\n", module->string);
    kprintf("[_start]   Module size: %lu bytes\n", module_size);
    kprintf("\n");

    pmm_init(boot_info);                /* reads memory map and initializes memory manager */
    
    u64 k_size = ((u64)&k_end - (u64)&k_start);
    kprintf("[_start]   Kernel size is %d bytes (0x%x)\n", k_size, k_size);


    //screen_init(boot_info);
    pit_init(1000);

    idt_init();

    enable_sce();

    cli();
	multitasking_init();
    sti();

    //to_userspace(&userspace_func, (void*)&user_stack[4095]);

    // We're done, just hang...
    hang();
}

