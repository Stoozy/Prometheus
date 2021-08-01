#include "pit.h"
#include <typedefs.h>
#include <cpu/io.h>
#include <kprintf.h>

static volatile u64 g_ticks = 0;

void pit_init(u32 hz){
    /* PIT mode 0 (interrupt on terminal count)/ channel 0 */
    outb(0x43, 0x0);

    u32 divisor = 1193182 / hz;

    outb(0x43, 0x36);
    outb(0x40, (u8)(divisor & 0x00ff));
    outb(0x40, (u8)((divisor & 0xff00) >> 8));
}

void sleep(u32 ms){
    /* loop until estimated ticks reached */
    u64 eticks = g_ticks+ms;
    kprintf("Sleeping: %d\n", g_ticks);
    while(g_ticks < eticks){
        asm("nop");
    }
}

/* called by irq0 */
void tick(){ 
    kprintf("Ticks: %d\n", g_ticks);
    ++g_ticks; 
}

