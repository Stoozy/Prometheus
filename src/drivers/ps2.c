#include "ps2.h"
#include <cpu/io.h>

#define PS2_DATA            0x60
#define PS2_PORT            0x64

#define PS2_DISABLE_FIRST       0xAD
#define PS2_DISABLE_SECOND      0xA7

#define PS2_ENABLE_FIRST        0xAE
#define PS2_ENABLE_SECOND       0xA8


void ps2_init(){
    // disable ps2 ports
    outb(PS2_PORT, PS2_DISABLE_FIRST);
    outb(PS2_PORT, PS2_DISABLE_SECOND);

    // flush buffer
    while(inb(PS2_DATA));

    // enable ports 
    outb(PS2_PORT, PS2_ENABLE_FIRST);
    outb(PS2_PORT, PS2_ENABLE_SECOND);
}



