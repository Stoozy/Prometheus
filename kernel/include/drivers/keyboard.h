#pragma once
#include <stdint.h>
#include <typedefs.h>

uint8_t kbd_read_from_buffer();
void handle_scan(u8 scan_code);

