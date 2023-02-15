#include "misc/initrd.h"
#define NULL (void *)0

#include "fs/vfs.h"
#include <cpu/gdt.h>
#include <cpu/idt.h>
#include <cpu/io.h>
#include <cpu/smp.h>
#include <libk/kmalloc.h>
#include <libk/typedefs.h>
#include <memory/pmm.h>
#include <memory/vmm.h>

#include <drivers/fb.h>
#include <drivers/keyboard.h>
#include <drivers/pit.h>
#include <drivers/serial.h>

#include <libk/kprintf.h>
#include <libk/util.h>

#include <proc/elf.h>
#include <proc/proc.h>

#include <stivale2.h>
#include <string/string.h>
#include <syscall/syscalls.h>

#include <drivers/tty.h>
#include <syscall/syscalls.h>

#include <abi-bits/fcntl.h>
#include <fs/tmpfs.h>

u64 k_start;
u64 k_end;
u64 k_size;

extern void load_pagedir(PageTable *);
extern void invalidate_tlb();
extern void gdt_init();
// We need to tell the stivale bootloader where we want our stack to be.
// We are going to allocate our stack as an uninitialised array in .bss.
volatile u8 stack[4096];

// We are now going to define a framebuffer header tag.
// This tag tells the bootloader that we want a graphical framebuffer instead
// of a CGA-compatible text mode. Omitting this tag will make the bootloader
// default to text mode, if available.
static struct stivale2_header_tag_framebuffer framebuffer_hdr_tag = {
    // Same as above.
    .tag =
        {.identifier = STIVALE2_HEADER_TAG_FRAMEBUFFER_ID,
         // Instead of 0, we now point to the previous header tag. The order in
         // which header tags are linked does not matter.
         .next = 0},
    // We set all the framebuffer specifics to 0 as we want the bootloader
    // to pick the best it can.
    .framebuffer_width = 0,
    .framebuffer_height = 0,
    .framebuffer_bpp = 0};

static struct stivale2_struct_tag_smp smp_hdr_tag = {
    // All tags need to begin with an identifier and a pointer to the next tag.
    .tag =
        {// Identification constant defined in stivale2.h and the specification.
         .identifier = STIVALE2_HEADER_TAG_SMP_ID,
         // If next is 0, it marks the end of the linked list of header tags.
         .next = (uint64_t)&framebuffer_hdr_tag},

};

// The stivale2 specification says we need to define a "header structure".
// This structure needs to reside in the .stivale2hdr ELF section in order
// for the bootloader to find it. We use this __attribute__ directive to
// tell the compiler to put the following structure in said section.
__attribute__((section(".stivale2hdr"),
               used)) static struct stivale2_header stivale_hdr = {
    // The entry_point member is used to specify an alternative entry
    // point that the bootloader should jump to instead of the executable's
    // ELF entry point. We do not care about that so we leave it zeroed.
    .entry_point = 0,
    // Let's tell the bootloader where our stack is.
    // We need to add the sizeof(stack) since in x86(_64) the stack grows
    // downwards.
    .stack = (uintptr_t)stack + sizeof(stack),
    // Bit 1, if set, causes the bootloader to return to us pointers in the
    // higher half, which we likely want since this is a higher half kernel.
    // Bit 2, if set, tells the bootloader to enable protected memory ranges,
    // that is, to respect the ELF PHDR mandated permissions for the
    // executable's
    // segments.
    // Bit 3, if set, enables fully virtual kernel mappings, which we want as
    // they allow the bootloader to pick whichever *physical* memory address is
    // available to load the kernel, rather than relying on us telling it where
    // to load it.
    // Bit 4 disables a deprecated feature and should always be set.
    .flags = (1 << 1) | (0 << 2) | (0 << 3) | (1 << 4),
    // This header structure is the root of the linked list of header tags and
    // points to the first one in the linked list.
    .tags = (uintptr_t)&smp_hdr_tag};

// We will now write a helper function which will allow us to scan for tags
// that we want FROM the bootloader (structure tags).
void *stivale2_get_tag(struct stivale2_struct *stivale2_struct, uint64_t id) {
  struct stivale2_tag *current_tag = (void *)stivale2_struct->tags;
  for (;;) {
    // If the tag pointer is NULL (end of linked list), we did not find
    // the tag. Return NULL to signal this.
    if (current_tag == NULL) {
      return NULL;
    }

    // Check whether the identifier matches. If it does, return a pointer
    // to the matching tag.
    if (current_tag->identifier == id) {
      return current_tag;
    }

    // Get a pointer to the next tag in the linked list and repeat.
    current_tag = (void *)current_tag->next;
  }
}

extern void enable_sce();

void panic(const char *msg) {
  kprintf("Kernel panic. Reason: %s \n", msg);

  for (;;)
    ;
}

void _start(struct stivale2_struct *boot_info) {
  disable_irq();

  serial_init(); /* init debugging */

  struct stivale2_struct_tag_memmap *meminfo =
      stivale2_get_tag(boot_info, STIVALE2_STRUCT_TAG_MEMMAP_ID);
  struct stivale2_struct_tag_modules *modules_tag =
      stivale2_get_tag(boot_info, STIVALE2_STRUCT_TAG_MODULES_ID);
  struct stivale2_struct_tag_framebuffer *framebuffer_tag =
      stivale2_get_tag(boot_info, STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID);

  pmm_init(meminfo);
  vmm_init();
  kmalloc_init(0xff0000);

  gdt_init();
  idt_init();
  pit_init(1000);

  extern void sse_init();
  sse_init();

  if (tmpfs_init())
    panic("Failed to initialize tmpfs\n");

  if (load_initrd(modules_tag))
    panic("Failed to read ramdisk... ");

  if (devfs_init())
    panic("Failed to mount devfs");

  kprintf("Loaded initrd\n");

  if (!framebuffer_tag)
    panic("No available framebuffer\n");

  if (fb_init(framebuffer_tag))
    panic("Failed to initialize framebuffer\n");
  for (;;)
    ;

  // kbd_init();
  // tty_init();
  // pty_init();

  // extern int input_init();
  // input_init();

  sys_init();
  multitasking_init();
}
