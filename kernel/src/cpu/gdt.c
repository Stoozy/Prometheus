#include "gdt.h"
#include "../string/string.h"
#include "../kprintf.h"

extern void load_gdt(GdtPtr *);

volatile GdtEntry gdt[8]; 
volatile Tss g_tss;
GdtPtr gdt_ptr;

void gdt_init(){

    /* null segment */
    gdt[0] = (GdtEntry) { .limit_lo = 0, .base_lo = 0, .base_mid = 0, 
        .access=0x0, .limit_and_flags=0x0, .base_hi=0};

    /* kernel code and data */
    gdt[1] = (GdtEntry) { .limit_lo = 0, .base_lo = 0, .base_mid = 0, 
        .access=0x9a, .limit_and_flags=0xa0, .base_hi=0};               /* kernel code 0x08 */

    gdt[2] = (GdtEntry) { .limit_lo = 0, .base_lo = 0, .base_mid = 0, 
        .access=0x92, .limit_and_flags=0xa0, .base_hi=0};               /* kernel data 0x10 */

    /* user code and data */

    /* null (user base selector) */
    gdt[3] = (GdtEntry) { .limit_lo = 0, .base_lo = 0, .base_mid = 0, 
        .access=0x0, .limit_and_flags=0x0, .base_hi=0};

    gdt[4] = (GdtEntry) { .limit_lo = 0, .base_lo = 0, .base_mid = 0, 
        .access=0x92, .limit_and_flags=0xa0, .base_hi=0};               /* 0x20 user data */

    gdt[5] = (GdtEntry) { .limit_lo = 0, .base_lo = 0, .base_mid = 0, 
        .access=0x9a, .limit_and_flags=0xa0, .base_hi=0};               /* 0x28 user code */

    /* tss low and high */
    gdt[6] = (GdtEntry) { .limit_lo = 0, .base_lo = 0, .base_mid = 0, 
        .access=0x89, .limit_and_flags=0xa0, .base_hi=0};               /* tss 0x40 */

    gdt[7] = (GdtEntry) { .limit_lo = 0, .base_lo = 0, .base_mid = 0, 
        .access=0x0, .limit_and_flags=0x0, .base_hi=0};

    memset((void*)&g_tss,0, sizeof(Tss));

    u64 tss_base = ((u64)&g_tss);

    gdt[6].base_lo = tss_base & 0xffff;
    gdt[6].base_mid = (tss_base >> 16) & 0xff;
    gdt[6].base_hi = (tss_base >> 24) & 0xff;
    gdt[6].limit_lo = sizeof(Tss);

    gdt[7].limit_lo = (tss_base >> 32) & 0xffff;
    gdt[7].base_lo = (tss_base >> 48) & 0xffff;

    kprintf("[GDT] GDT Address: 0x%x\n", &gdt);
    kprintf("[GDT] GDT Pointer size: %lu bytes\n", sizeof(GdtPtr));

    gdt_ptr.limit = sizeof(gdt)-1;
    gdt_ptr.base = (u64)&gdt[0];

    kprintf("[GDT] GdtPtr Address: 0x%x\n", &gdt_ptr);

    load_gdt(&gdt_ptr);
}