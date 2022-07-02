#pragma once
#include <typedefs.h>

typedef struct {
    u16 limit;
    u64 base;
} __attribute__((packed)) IDTPtr;

typedef struct {
    u16 offset_lo;
    u16 selector;
    u8 zero;
    u8 type_attr;
    u16 offset_mid;
    u32 offset_hi;
    u32 rsv0;
} __attribute__((packed)) IDTEntry;



void idt_set_descriptor(u8 vector, u64 isr, u8 flags);
void idt_init();
void Sleep(u32 ms);

