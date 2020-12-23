#include <kernel/idt.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct IDT_D idt_desc;
typedef struct IDT_PTR idt_ptr;

idt_desc IDT[256];
idt_ptr ip;



static inline void io_wait(void)
{
    /* TODO: This is probably fragile. */
    asm volatile ( "jmp 1f\n\t"
                   "1:jmp 2f\n\t"
                   "2:" );
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}


void init_idt(){

// initialize interrupt descriptor table
	//for(int i=0; i<256; i++){
	//	init_idt_desc((uint32_t) &entries[i], 0x808, 0x8E, entries[i]);
	//}

	extern uint32_t load_idt();
	extern uint32_t irq0();
	extern uint32_t irq1();
	extern uint32_t irq1();
	extern uint32_t irq2();
	extern uint32_t irq3();
	extern uint32_t irq4();
	extern uint32_t irq5();
	extern uint32_t irq6();
	extern uint32_t irq7();
	extern uint32_t irq8();
	extern uint32_t irq9();
	extern uint32_t irq10();
	extern uint32_t irq11();
	extern uint32_t irq12();
	extern uint32_t irq13();
	extern uint32_t irq14();
	extern uint32_t irq15();

    
	uint64_t irq0_address;
	uint64_t irq1_address;
	uint64_t irq2_address;
	uint64_t irq3_address;          
	uint64_t irq4_address; 
	uint64_t irq5_address;
	uint64_t irq6_address;
	uint64_t irq7_address;
	uint64_t irq8_address;
	uint64_t irq9_address;          
	uint64_t irq10_address;
	uint64_t irq11_address;
	uint64_t irq12_address;
	uint64_t irq13_address;
	uint64_t irq14_address;          
	uint64_t irq15_address;         
	uint64_t idt_address;
	uint64_t idt_ptr[2];	

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

	irq0_address = (unsigned long)irq0; 
	IDT[32].offset_1 = irq0_address & 0xffff;
	IDT[32].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	IDT[32].zero = 0;
	IDT[32].type_attr = 0x8e; /* INTERRUPT_GATE */
	IDT[32].offset_2 = (irq0_address & 0xffff0000) >> 16;
 
	irq1_address = (unsigned long)irq1; 
	IDT[33].offset_1 = irq1_address & 0xffff;
	IDT[33].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	IDT[33].zero = 0;
	IDT[33].type_attr = 0x8e; /* INTERRUPT_GATE */
	IDT[33].offset_2 = (irq1_address & 0xffff0000) >> 16;
 
	irq2_address = (unsigned long)irq2; 
	IDT[34].offset_1 = irq2_address & 0xffff;
	IDT[34].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	IDT[34].zero = 0;
	IDT[34].type_attr = 0x8e; /* INTERRUPT_GATE */
	IDT[34].offset_2 = (irq2_address & 0xffff0000) >> 16;
 
	irq3_address = (unsigned long)irq3; 
	IDT[35].offset_1 = irq3_address & 0xffff;
	IDT[35].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	IDT[35].zero = 0;
	IDT[35].type_attr = 0x8e; /* INTERRUPT_GATE */
	IDT[35].offset_2 = (irq3_address & 0xffff0000) >> 16;
 
	irq4_address = (unsigned long)irq4; 
	IDT[36].offset_1 = irq4_address & 0xffff;
	IDT[36].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	IDT[36].zero = 0;
	IDT[36].type_attr = 0x8e; /* INTERRUPT_GATE */
	IDT[36].offset_2 = (irq4_address & 0xffff0000) >> 16;
 
	irq5_address = (unsigned long)irq5; 
	IDT[37].offset_1 = irq5_address & 0xffff;
	IDT[37].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	IDT[37].zero = 0;
	IDT[37].type_attr = 0x8e; /* INTERRUPT_GATE */
	IDT[37].offset_2 = (irq5_address & 0xffff0000) >> 16;
 
	irq6_address = (unsigned long)irq6; 
	IDT[38].offset_1 = irq6_address & 0xffff;
	IDT[38].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	IDT[38].zero = 0;
	IDT[38].type_attr = 0x8e; /* INTERRUPT_GATE */
	IDT[38].offset_2 = (irq6_address & 0xffff0000) >> 16;
 
	irq7_address = (unsigned long)irq7; 
	IDT[39].offset_1 = irq7_address & 0xffff;
	IDT[39].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	IDT[39].zero = 0;
	IDT[39].type_attr = 0x8e; /* INTERRUPT_GATE */
	IDT[39].offset_2 = (irq7_address & 0xffff0000) >> 16;
 
	irq8_address = (unsigned long)irq8; 
	IDT[40].offset_1 = irq8_address & 0xffff;
	IDT[40].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	IDT[40].zero = 0;
	IDT[40].type_attr = 0x8e; /* INTERRUPT_GATE */
	IDT[40].offset_2 = (irq8_address & 0xffff0000) >> 16;
 
	irq9_address = (unsigned long)irq9; 
	IDT[41].offset_1 = irq9_address & 0xffff;
	IDT[41].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	IDT[41].zero = 0;
	IDT[41].type_attr = 0x8e; /* INTERRUPT_GATE */
	IDT[41].offset_2 = (irq9_address & 0xffff0000) >> 16;
 
	irq10_address = (unsigned long)irq10; 
	IDT[42].offset_1 = irq10_address & 0xffff;
	IDT[42].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	IDT[42].zero = 0;
	IDT[42].type_attr = 0x8e; /* INTERRUPT_GATE */
	IDT[42].offset_2 = (irq10_address & 0xffff0000) >> 16;
 
	irq11_address = (unsigned long)irq11; 
	IDT[43].offset_1 = irq11_address & 0xffff;
	IDT[43].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	IDT[43].zero = 0;
	IDT[43].type_attr = 0x8e; /* INTERRUPT_GATE */
	IDT[43].offset_2 = (irq11_address & 0xffff0000) >> 16;
 
	irq12_address = (unsigned long)irq12; 
	IDT[44].offset_1 = irq12_address & 0xffff;
	IDT[44].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	IDT[44].zero = 0;
	IDT[44].type_attr = 0x8e; /* INTERRUPT_GATE */
	IDT[44].offset_2 = (irq12_address & 0xffff0000) >> 16;
 
	irq13_address = (unsigned long)irq13; 
	IDT[45].offset_1 = irq13_address & 0xffff;
	IDT[45].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	IDT[45].zero = 0;
	IDT[45].type_attr = 0x8e; /* INTERRUPT_GATE */
	IDT[45].offset_2 = (irq13_address & 0xffff0000) >> 16;
 
	irq14_address = (unsigned long)irq14; 
	IDT[46].offset_1 = irq14_address & 0xffff;
	IDT[46].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	IDT[46].zero = 0;
	IDT[46].type_attr = 0x8e; /* INTERRUPT_GATE */
	IDT[46].offset_2 = (irq14_address & 0xffff0000) >> 16;
 
	irq15_address = (unsigned long)irq15; 
	IDT[47].offset_1 = irq15_address & 0xffff;
	IDT[47].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	IDT[47].zero = 0;
	IDT[47].type_attr = 0x8e; /* INTERRUPT_GATE */
	IDT[47].offset_2 = (irq15_address & 0xffff0000) >> 16;
 
	/* fill the IDT descriptor */
	idt_address = (unsigned long)IDT ;
	idt_ptr[0] = (sizeof(struct IDT_D) * 256) + ((idt_address & 0xffff) << 16);
	idt_ptr[1] = idt_address >> 16 ;
 
 
 
	//load_idt(idt_ptr);
    //load_idt();
	// load the idt
	//asm volatile("lidt %0" : :"g" (&idt_ptr));
	printf("[kernel] IDT Loaded\n\n");
}


//void init_idt_desc(uint32_t offset, uint16_t selector, uint8_t type, idt_desc src){
//    // initialize entry
//	src.offset_1 =  (uint16_t) offset;
//	src.offset_2  = (uint16_t) (offset<<16);
//	src.selector = selector;
//	src.type_attr = type;
//    
//}

bool are_interrupts_enabled(){
        unsigned long flags;
        asm volatile ( "pushf\n\t"
                       "pop %0"
                      : "=g"(flags) );
        return flags & (1 << 9);
}

void irq0_handler(void) {
	outb(0x20, 0x20); //EOI
}

void irq1_handler(void) {	
    uint8_t c = inb(0x60);
    putchar(c);
	outb(0x20, 0x20); //EOI
}

void irq2_handler(void) {
  outb(0x20, 0x20); //EOI
}

void irq3_handler(void) {
  outb(0x20, 0x20); //EOI
}

void irq4_handler(void) {
  outb(0x20, 0x20); //EOI
}

void irq5_handler(void) {
  outb(0x20, 0x20); //EOI
}

void irq6_handler(void) {
  outb(0x20, 0x20); //EOI
}

void irq7_handler(void) {
  outb(0x20, 0x20); //EOI
}

void irq8_handler(void) {
  outb(0xA0, 0x20);
  outb(0x20, 0x20); //EOI          
}

void irq9_handler(void) {
  outb(0xA0, 0x20);
  outb(0x20, 0x20); //EOI
}

void irq10_handler(void) {
  outb(0xA0, 0x20);
  outb(0x20, 0x20); //EOI
}

void irq11_handler(void) {
  outb(0xA0, 0x20);
  outb(0x20, 0x20); //EOI
}

void irq12_handler(void) {
  outb(0xA0, 0x20);
  outb(0x20, 0x20); //EOI
}

void irq13_handler(void) {
  outb(0xA0, 0x20);
  outb(0x20, 0x20); //EOI
}

void irq14_handler(void) {
  outb(0xA0, 0x20);
  outb(0x20, 0x20); //EOI
}

void irq15_handler(void) {
  outb(0xA0, 0x20);
  outb(0x20, 0x20); //EOI
}


/* reinitialize the PIC controllers, giving them specified vector offsets
   rather than 8h and 70h, as configured by default */


/*
arguments:
	offset1 - vector offset for master PIC
		vectors on the master become offset1..offset1+7
	offset2 - same for slave PIC: offset2..offset2+7
*/
void init_pic(uint32_t offset1, uint32_t offset2)
{
	unsigned char a1, a2;
 
	a1 = inb(PIC1_DATA);                        // save masks
	a2 = inb(PIC2_DATA);
 
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
	io_wait();
	outb(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
	io_wait();
	outb(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	io_wait();
	outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();
 
	outb(PIC1_DATA, ICW4_8086);
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();
 
	outb(PIC1_DATA, a1);   // restore saved masks.
	outb(PIC2_DATA, a2);
    printf("[kernel] PIC initialized\n\n");
    return;
}

