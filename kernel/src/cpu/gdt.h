#pragma once

#include "../typedefs.h"

typedef struct {
    u64 base;
    u16 limit;
} GdtPtr;

typedef struct  {
    u16 limit_lo;
    u16 base_lo;
    u8 base_mid;
    u8 access;
    u8 limit_and_flags; /* first 4 bits : limit then 4 bit flags */
    u8 base_hi;
} __attribute__((packed)) GdtEntry; 


typedef struct {
    u32 rsvd0;

    u64 rsp0;
    u64 rsp1;
    u64 rsp2;

    u64 rsvd1;

    u64 ist1;
    u64 ist2;
    u64 ist3;
    u64 ist4;
    u64 ist5;
    u64 ist6;
    u64 ist7;
    u64 rsvd2;
    u64 rsvd3;

    u64 iomap_base;

} __attribute__((packed)) Tss;


void gdt_init();