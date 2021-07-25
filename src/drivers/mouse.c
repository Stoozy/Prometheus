#include <cpu/io.h>
#include <kprintf.h>

void mouse_wait(){
    u32 timeout = 10000;
    while(timeout--)
        if(inb(0x64) & 0) return;
}

void mouse_init(){
    kprintf("Initializing mouse driver\n");

    outb(0x64, 0xA8);
    mouse_wait();
    outb(0x64, 0x20);
    mouse_wait();

    u8 status = inb(0x64);

    kprintf("Mouse status 0x%x\n", status);

    status |= 2; /* enable IRQ12 */
    status &= ~(1 << 5); /* clear bit 5*/

    kprintf("Modified mouse status 0x%x\n", status);
    outb(0x64, 0x60);
    outb(0x64, status); /* send modified status byte */

}
