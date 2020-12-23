#include "typedefs.h"
#include <stddef.h>

#define GDT_BASE 0x800

#ifndef GDT_H
#define GDT_H


void init_gdt_desc(int w, uint32_t base, uint32_t lim, uint8_t access, uint8_t gran);
void init_gdt();


struct tss{
    uint16_t link;
    uint16_t link_res;
    uint32_t esp0;
    uint16_t ss0;
    uint16_t ss0_res;
    uint16_t esp1;
    uint16_t ss1;
    uint16_t ss1_res;
    uint16_t esp2;
    uint16_t ss2;
    uint16_t ss2_res;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint16_t es;
    uint16_t es_res;
    uint16_t cs;
    uint16_t cs_res;
    uint16_t ss;
    uint16_t ss_res;
    uint16_t ds;
    uint16_t ds_res;
    uint16_t fs;
    uint16_t fs_res;
    uint16_t gs;
    uint16_t gs_res;
    uint16_t LDTR;
    uint16_t LDTR_RES;
    uint16_t IOPB_RES;
    uint16_t IOPB;
} __attribute__ ((packed));

struct gdtr{
    uint16_t limit;
    uint32_t base;
} __attribute__ ((packed));


struct gdtdesc{
    uint16_t lima;          // lim part a 0:15
    uint16_t basea;         // base part a 16:31
    uint8_t  baseb;         // base part b 32:39
    uint8_t  ab;            // access byte 40:47
    uint8_t  limflags;      // lim part b 48:51 and flags 52:55
    uint8_t basec;          // base part c 56:63

} __attribute__ ((packed));


#endif
