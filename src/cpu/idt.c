#include "idt.h"
#include <drivers/serial.h>
#include <cpu/io.h>
#include <kprintf.h>

IDTEntry idt[256];

void idt_init(){
    kprintf("Initializing IDT\n");

    extern int load_idt();
    extern int irq0(); 
    extern int irq1(); 
    extern int irq2(); 
    extern int irq3(); 
    extern int irq4(); 
    extern int irq5(); 
    extern int irq6(); 
    extern int irq7(); 
    extern int irq8(); 
    extern int irq9(); 
    extern int irq10(); 
    extern int irq11(); 
    extern int irq12(); 
    extern int irq13(); 
    extern int irq14(); 
    extern int irq15(); 

    u32 irq0_addr;
    u32 irq1_addr;
    u32 irq2_addr;
    u32 irq3_addr;
    u32 irq4_addr;
    u32 irq5_addr;
    u32 irq6_addr;
    u32 irq7_addr;
    u32 irq8_addr;
    u32 irq9_addr;
    u32 irq10_addr;
    u32 irq11_addr;
    u32 irq12_addr;
    u32 irq13_addr;
    u32 irq14_addr;
    u32 irq15_addr;

    u32 idt_ptr[2];

	/* remapping the PIC */
	outb(0x20, 0x11);
	outb(0xA0, 0x11);
	outb(0x21, 0x20);
	outb(0xA1, 40);
	outb(0x21, 0x04);
	outb(0xA1, 0x02);
	outb(0x21, 0x01);
	outb(0xA1, 0x01);
	outb(0x21, 0x0);
	outb(0xA1, 0x0);

    irq0_addr = (unsigned long)irq0;
	idt[32].offset_lo = irq0_addr & 0xffff;
	idt[32].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	idt[32].zero = 0;
	idt[32].type_attr = 0x8e; /* INTERRUPT_GATE */
	idt[32].offset_hi = (irq0_addr & 0xffff0000) >> 16;


    irq1_addr = (unsigned long)irq1;
	idt[33].offset_lo = irq1_addr & 0xffff;
	idt[33].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	idt[33].zero = 0;
	idt[33].type_attr = 0x8e; /* INTERRUPT_GATE */
	idt[33].offset_hi = (irq1_addr & 0xffff0000) >> 16;


    irq2_addr = (unsigned long)irq2;
	idt[34].offset_lo = irq2_addr & 0xffff;
	idt[34].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	idt[34].zero = 0;
	idt[34].type_attr = 0x8e; /* INTERRUPT_GATE */
	idt[34].offset_hi = (irq2_addr & 0xffff0000) >> 16;


    irq3_addr = (unsigned long)irq3;
	idt[35].offset_lo = irq3_addr & 0xffff;
	idt[35].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	idt[35].zero = 0;
	idt[35].type_attr = 0x8e; /* INTERRUPT_GATE */
	idt[35].offset_hi = (irq3_addr & 0xffff0000) >> 16;


    irq4_addr = (unsigned long)irq4;
	idt[36].offset_lo = irq4_addr & 0xffff;
	idt[36].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	idt[36].zero = 0;
	idt[36].type_attr = 0x8e; /* INTERRUPT_GATE */
	idt[36].offset_hi = (irq4_addr & 0xffff0000) >> 16;


    irq5_addr = (unsigned long)irq5;
	idt[37].offset_lo = irq5_addr & 0xffff;
	idt[37].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	idt[37].zero = 0;
	idt[37].type_attr = 0x8e; /* INTERRUPT_GATE */
	idt[37].offset_hi = (irq5_addr & 0xffff0000) >> 16;


    irq6_addr = (unsigned long)irq6;
	idt[38].offset_lo = irq6_addr & 0xffff;
	idt[38].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	idt[38].zero = 0;
	idt[38].type_attr = 0x8e; /* INTERRUPT_GATE */
	idt[38].offset_hi = (irq6_addr & 0xffff0000) >> 16;


    irq7_addr = (unsigned long)irq7;
	idt[39].offset_lo = irq7_addr & 0xffff;
	idt[39].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	idt[39].zero = 0;
	idt[39].type_attr = 0x8e; /* INTERRUPT_GATE */
	idt[39].offset_hi = (irq7_addr & 0xffff0000) >> 16;


    irq8_addr = (unsigned long)irq8;
	idt[40].offset_lo = irq8_addr & 0xffff;
	idt[40].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	idt[40].zero = 0;
	idt[40].type_attr = 0x8e; /* INTERRUPT_GATE */
	idt[40].offset_hi = (irq8_addr & 0xffff0000) >> 16;


    irq9_addr = (unsigned long)irq9;
	idt[41].offset_lo = irq9_addr & 0xffff;
	idt[41].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	idt[41].zero = 0;
	idt[41].type_attr = 0x8e; /* INTERRUPT_GATE */
	idt[41].offset_hi = (irq9_addr & 0xffff0000) >> 16;


    irq10_addr = (unsigned long)irq10;
	idt[42].offset_lo = irq10_addr & 0xffff;
	idt[42].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	idt[42].zero = 0;
	idt[42].type_attr = 0x8e; /* INTERRUPT_GATE */
	idt[42].offset_hi = (irq10_addr & 0xffff0000) >> 16;


    irq11_addr = (unsigned long)irq11;
	idt[43].offset_lo = irq11_addr & 0xffff;
	idt[43].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	idt[43].zero = 0;
	idt[43].type_attr = 0x8e; /* INTERRUPT_GATE */
	idt[43].offset_hi = (irq11_addr & 0xffff0000) >> 16;


    irq12_addr = (unsigned long)irq12;
	idt[44].offset_lo = irq12_addr & 0xffff;
	idt[44].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	idt[44].zero = 0;
	idt[44].type_attr = 0x8e; /* INTERRUPT_GATE */
	idt[44].offset_hi = (irq12_addr & 0xffff0000) >> 16;


    irq13_addr = (unsigned long)irq13;
	idt[45].offset_lo = irq13_addr & 0xffff;
	idt[45].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	idt[45].zero = 0;
	idt[45].type_attr = 0x8e; /* INTERRUPT_GATE */
	idt[45].offset_hi = (irq13_addr & 0xffff0000) >> 16;


    irq14_addr = (unsigned long)irq14;
	idt[46].offset_lo = irq14_addr & 0xffff;
	idt[46].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	idt[46].zero = 0;
	idt[46].type_attr = 0x8e; /* INTERRUPT_GATE */
	idt[46].offset_hi = (irq14_addr & 0xffff0000) >> 16;


    irq15_addr = (unsigned long)irq15;
	idt[47].offset_lo = irq15_addr & 0xffff;
	idt[47].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	idt[47].zero = 0;
	idt[47].type_attr = 0x8e; /* INTERRUPT_GATE */
	idt[47].offset_hi = (irq15_addr & 0xffff0000) >> 16;
    
    /* fill the IDT descriptor */
	u32 idt_address = (unsigned long)idt;
	idt_ptr[0] = (sizeof(IDTEntry) * 256) + ((idt_address & 0xffff) << 16);
	idt_ptr[1] = idt_address >> 16 ;


    load_idt(idt_ptr);

}

