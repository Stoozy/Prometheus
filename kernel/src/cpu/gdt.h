#pragma once

#include "../typedefs.h"

struct table_ptr {
    u16 limit;
    u64 base;
} __attribute__((packed));


void gdt_init();
void gdt_reload();


