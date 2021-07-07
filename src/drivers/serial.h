#ifndef SERIAL_DRIVER_H
#define SERIAL_DRIVER_H

#include <typedefs.h>
#include <cpu/io.h>

#define SERIAL_PORT 0x3f8

int serial_init();

int serial_received();
char read_serial();

int is_transmit_empty();
void write_serial(char a);

void turn_color_on();
void turn_color_off();

#endif
