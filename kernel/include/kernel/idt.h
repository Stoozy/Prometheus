#include "typedefs.h"

struct IDT_D {
   uint16_t offset_1; // offset bits 0..15
   uint16_t selector; // a code segment selector in GDT or LDT
   uint8_t zero;      // unused, set to 0
   uint8_t type_attr; // type and attributes, see below
   uint16_t offset_2; // offset bits 16..31
}__attribute__ ((packed));

struct IDT_PTR {
    uint16_t size;
    uint32_t idt_start;
} __attribute__ ((packed));

void init_idt_desc(uint32_t offset, uint16_t selector, uint8_t type, struct IDT_D src);
void init_idt();
