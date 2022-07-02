#define SSFN_CONSOLEBITMAP_TRUECOLOR
#define NULL (void *)0

#include <cpu/gdt.h>
#include <cpu/idt.h>
#include <cpu/io.h>
#include <cpu/smp.h>
#include <kmalloc.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <misc/ssfn.h>
#include <typedefs.h>

#include <drivers/pit.h>
#include <drivers/serial.h>
#include <drivers/video.h>

#include <kprintf.h>
#include <util.h>

#include <proc/elf.h>
#include <proc/proc.h>

#include <fs/tarfs.h>
#include <stivale2.h>
#include <string/string.h>
#include <syscall/syscalls.h>

#include <syscall/syscalls.h>

u64 k_start;
u64 k_end;
u64 k_size;

extern void load_pagedir(PageTable *);
extern void invalidate_tlb();
extern void gdt_init();
// We need to tell the stivale bootloader where we want our stack to be.
// We are going to allocate our stack as an uninitialised array in .bss.
volatile u8 stack[4096];
volatile u8 user_stack[4096];

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
extern void to_userspace(void *entry, void *stack);

void hang() {
  for (;;)
    ;
}

void panic() {
  kprintf("KERNEL PANIC\n");
  for (;;)
    ;
}

void _start(struct stivale2_struct *boot_info) {

  serial_init(); /* init debugging */

  struct stivale2_struct_tag_memmap *meminfo =
      stivale2_get_tag(boot_info, STIVALE2_STRUCT_TAG_MEMMAP_ID);
  struct stivale2_struct_tag_modules *modules_tag =
      stivale2_get_tag(boot_info, STIVALE2_STRUCT_TAG_MODULES_ID);

  extern void kernelStart;
  extern void kernelEnd;
  k_start = (uint64_t)&kernelStart;
  k_end = (uint64_t)&kernelEnd;
  k_size = ((k_end - k_start) / PAGE_SIZE) * PAGE_SIZE;

  kprintf("kernel start is 0x%x\n", k_start);
  kprintf("kernel size is 0x%x\n", k_size);

  pmm_init(meminfo);

  kmalloc_init(0xff0000);

  gdt_init();
  idt_init();

  pit_init(1000);

  extern void sse_init();
  sse_init();

  FileSystem *tarfs = NULL;
  if (modules_tag == NULL) {
    kprintf("[MAIN]   No modules found. Exiting.\n");
    hang();
  } else {
    u64 num_modules = modules_tag->module_count;
    kprintf("[MAIN]   Found %d modules\n", num_modules);
    for (u64 m = 0; m < num_modules; ++m) {
      struct stivale2_module module = modules_tag->modules[m];
      kprintf("[MAIN]   Module name: %s\n", module.string);
      kprintf("[MAIN]   Module start: %llx\n", module.begin);
      kprintf("[MAIN]   Module end: %llx\n", module.end);
      kprintf("\n");
      if (strcmp(module.string, "INITRAMFS") == 0) {
        kprintf("[MAIN]   Found INITRAMFS, initializing!\n");
        tarfs = tarfs_init((u8 *)module.begin);
        vfs_init(tarfs);
      }
    }
  }

  struct stivale2_struct_tag_framebuffer *framebuffer_tag =
      stivale2_get_tag(boot_info, STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID);

  if (framebuffer_tag == NULL) {
    kprintf("[MAIN]   No framebuffer found. Exiting.\n");
    hang();
  } else {
    kprintf("[MAIN]   Framebuffer found!\n");
    kprintf("[MAIN]   Framebuffer addr: 0x%llx\n",
            framebuffer_tag->framebuffer_addr);
    kprintf("[MAIN]   Framebuffer bpp: %d\n", framebuffer_tag->framebuffer_bpp);
    kprintf("[MAIN]   Framebuffer height: %d\n",
            framebuffer_tag->framebuffer_height);
    kprintf("[MAIN]   Framebuffer width: %d\n",
            framebuffer_tag->framebuffer_width);

#define SSFN_CONSOLEBITMAP_TRUECOLOR
    ssfn_dst.ptr = (u8 *)framebuffer_tag->framebuffer_addr;
    ssfn_dst.w = framebuffer_tag->framebuffer_width;
    ssfn_dst.h = framebuffer_tag->framebuffer_height;
    ssfn_dst.p = 4096;
    ssfn_dst.x = 100;
    ssfn_dst.y = 205;
    ssfn_dst.fg = 0xffffff;
    // screen_init(framebuffer_tag);
  }

  cli();
  // smp_tag == NULL ? kprintf("[SMP]  SMP tag was not found.\n") :
  // smp_init(smp_tag);
  sys_init();

  // jump to userspace
  multitasking_init();
}
