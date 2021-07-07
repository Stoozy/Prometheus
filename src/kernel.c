
#include "typedefs.h"

//#include <stdint.h>
#include <stddef.h>
#include <stivale.h>
#include <cpu/io.h>

#define PORT    0x3f8
// We need to tell the stivale bootloader where we want our stack to be.
// We are going to allocate our stack as an uninitialised array in .bss.
static u8 stack[4096];

// The stivale specification says we need to define a "header structure".
// This structure needs to reside in the .stivalehdr ELF section in order
// for the bootloader to find it. We use this __attribute__ directive to
// tell the compiler to put the following structure in said section.
__attribute__((section(".stivalehdr"), used))
static struct stivale_header stivale_hdr = {
    // Let's tell the bootloader where our stack is.
    // We need to add the sizeof(stack) since in x86(_64) the stack grows
    // downwards.
    .stack = (uintptr_t)stack + sizeof(stack),
    // These flags can be used to tell the bootloader to enable or disable
    // certain features. We turn on bit 0 as we are going to ask for a
    // graphical framebuffer. Not setting this bit will make the
    // bootloader default to a CGA-compatible text mode instead.
    // We also toggle bit 3 as that will make the bootloader offset pointers
    // given to us to the higher half.
    .flags = (1 << 0) | (1 << 3),
    // We set all the framebuffer specifics to 0 as we want the bootloader
    // to pick the best it can.
    .framebuffer_width  = 0,
    .framebuffer_height = 0,
    .framebuffer_bpp    = 0,
    // The entry_point member is used to specify an alternative entry
    // point that the bootloader should jump to instead of the executable's
    // ELF entry point. We do not care about that so we leave it zeroed.
    .entry_point = 0
};

static int init_serial() {
   outb(PORT + 1, 0x00);    // Disable all interrupts
   outb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
   outb(PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
   outb(PORT + 1, 0x00);    //                  (hi byte)
   outb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
   outb(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
   outb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
   outb(PORT + 4, 0x1E);    // Set in loopback mode, test the serial chip
   outb(PORT + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

   // Check if serial is faulty (i.e: not same byte as sent)
   if(inb(PORT + 0) != 0xAE) {
      return 1;
   }

   // If serial is not faulty set it in normal operation mode
   // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
   outb(PORT + 4, 0x0F);
   return 0;
}

int is_transmit_empty() {
   return inb(PORT + 5) & 0x20;
}

void write_serial(char a) {
   while (is_transmit_empty() == 0){
       asm("nop");
   }
 
   outb(PORT,a);
}

// The following will be our kernel's entry point.
void _start(struct stivale_struct *stivale_struct) {
    // Let's get the address of the framebuffer.
    u8 *fb_addr = (u8 *)stivale_struct->framebuffer_addr;
    // Let's try to paint a few pixels white in the top left, so we know
    // that we booted correctly.
    for (u64 i = 0; i < 1024*768; i++) {
        fb_addr[i] = 0xff;
    }

    if(init_serial()){
        write_serial('T');
    }
    asm("ret");
    

    // We're done, just hang...
    for (;;) {
        asm ("hlt");
    }
}
