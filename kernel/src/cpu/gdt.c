#include "gdt.h"
#include "../string/string.h"
#include "../kprintf.h"
#include "../memory/pmm.h"

extern void gdt_flush(u64);
extern void load_gdt(u64);

static TSS64 tss = {
    .reserved = 0,
    .rsp = {0, 0, 0},
    .reserved0 = 0,
    .ist = {0, 0, 0},
    .reserved1 = 0,
    .reserved2 = 0,
    .reserved3 = 0,
    .iopb_offset = 0,
};

static GDT64 gdt;

static GDTDescriptor64 gdt_descriptor = {
    .size = sizeof(GDT64) - 1,
    .offset = (u64)&gdt,
};

void set_entry(GDTEntry64 entry, u32 base, u32 limit, u8 gran, u8 flags){
    entry.limit0_15 = (u16)(limit & 0xffff);
    entry.base0_15 = (u16)(base & 0xffff);
    entry.base16_23 = (u8)((base >> 16) & 0xff);
    entry.flags = flags;
    entry.limit16_19 = (limit >> 16) & 0x0f;
    entry.granularity = gran;
    entry.base24_31 = (u8)((base>>24) & 0xff);
}

void set_tss_entry(GDTTSSEntry64 tss_entry, u64 tss_addr){
    tss_entry.length = sizeof(TSS64);
    tss_entry.base_low16 = tss_addr & 0xffff;
    tss_entry.base_mid8 = (tss_addr >> 16) & 0xff;
    tss_entry.flags1 = 0x89;
    tss_entry.flags2 = 0;
    tss_entry.base_high8 = (tss_addr >> 24) & 0xff;
    tss_entry.base_upper32 = (tss_addr >> 32);
    tss_entry.reserved = 0;
}

void set_kernel_stack(u64 stack)
{
    tss.rsp[0] = stack;
    tss.ist[0] = stack;
}

void gdt_init()
{
    // 0x00
    set_entry(gdt.entries[0], 0, 0, 0, 0);
    // 0x08
    set_entry(gdt.entries[1], 0, 0, GDT_LONG_MODE_GRANULARITY, GDT_PRESENT | GDT_SEGMENT | GDT_READWRITE | GDT_EXECUTABLE);
    // 0x10
    set_entry(gdt.entries[2], 0, 0, 0, GDT_PRESENT | GDT_SEGMENT | GDT_READWRITE | GDT_EXECUTABLE);
    // 0x18
    set_entry(gdt.entries[3], 0, 0, GDT_LONG_MODE_GRANULARITY, GDT_PRESENT | GDT_SEGMENT | GDT_READWRITE | GDT_EXECUTABLE | GDT_USER);
    // 0x20
    set_entry(gdt.entries[4], 0, 0, 0, GDT_PRESENT | GDT_SEGMENT | GDT_READWRITE | GDT_USER);
    // 0x28
    set_tss_entry(gdt.tss, (u64)&tss);

//    gdt.entries[0] = { 0, 0, 0, 0}; // null descriptor
//    gdt.entries[1] = {GDT_PRESENT | GDT_SEGMENT | GDT_READWRITE | GDT_EXECUTABLE, GDT_LONG_MODE_GRANULARITY};
//    gdt.entries[2] = {GDT_PRESENT | GDT_SEGMENT | GDT_READWRITE, 0};
//
//    gdt.entries[3] = {GDT_PRESENT | GDT_SEGMENT | GDT_READWRITE | GDT_EXECUTABLE | GDT_USER, GDT_LONG_MODE_GRANULARITY};
//    gdt.entries[4] = {GDT_PRESENT | GDT_SEGMENT | GDT_READWRITE | GDT_USER, 0};
//
    //gdt.tss = {(uintptr_t)&tss};
    
    set_kernel_stack((u64)pmm_alloc_block()+0x1000);
    gdt_flush((u64)&gdt_descriptor);
}



