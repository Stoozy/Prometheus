#pragma once
#include "../typedefs.h"

typedef struct {
    u16 offset_lo;
    u16 selector;
    u8 zero;
    u8 type_attr;
    u8 offset_mid;
    u32 offset_hi;
    u32 zero1;
} IDTEntry;

void idt_init();

