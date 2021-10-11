#pragma once
#include <stdint.h>

#include "../typedefs.h"

struct gdt_entry {
  uint16_t limit15_0;            uint16_t base15_0;
  uint8_t  base23_16;            uint8_t  type;
  uint8_t  limit19_16_and_flags; uint8_t  base31_24;
};

struct table_ptr {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed));

