#pragma once
#include <stdint.h>
#include <libk/typedefs.h>
#include <libk/ringbuffer.h>


void kbd_register_buffer(RingBuffer *buffer);
uint8_t kbd_read_from_buffer();
void handle_scan(u8 scan_code);
void kbd_init();

