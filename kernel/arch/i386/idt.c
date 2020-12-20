#include <kernel/idt.h>
#include <stdio.h>

typedef struct IDT_D idt_desc;
typedef struct IDT_PTR idt_ptr;

idt_desc entries[256];
idt_ptr ip;
void init_idt(){
    // initialize interrupt descriptor table
	uint32_t base = (uint32_t) &entries[0];
	for(int i=0; i<256; i++){
		init_idt_desc((uint32_t) &entries[i], 0x808, 0x8E, entries[i]);
	}
	ip.idt_start = (uint32_t)&entries[0];
	ip.size = sizeof(entries);

	// load the idt
	asm volatile("lidt %0" : :"m" (ip));
	printf("[kernel] IDT Loaded\n\n");
}

void init_idt_desc(uint32_t offset, uint16_t selector, uint8_t type, idt_desc src){
    // initialize entry
	src.offset_1 =  (uint16_t) offset;
	src.offset_2  = (uint16_t) (offset<<16);
	src.selector = selector;
	src.type_attr = type;
    
}

