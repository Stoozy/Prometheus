#include <cpu/idt.h>

#include <kprintf.h>
#include <typedefs.h>

#include <drivers/pit.h>
#include <drivers/serial.h>

#include <cpu/cpu.h>
#include <cpu/io.h>
#include <drivers/keyboard.h>
#include <proc/proc.h>

__attribute__((aligned(0x10))) IDTEntry idt[256];

void idt_set_descriptor(u8 vector, u64 isr, u8 flags) {

  IDTEntry *desc = &idt[vector];
  desc->offset_lo = (u64)isr & 0xFFFF;
  desc->selector = 0x08;
  desc->zero = 0;
  desc->type_attr = flags;
  desc->offset_mid = (isr >> 16) & 0xFFFF;
  desc->offset_hi = (isr >> 32) & 0xFFFFFFFF;
  desc->rsv0 = 0;
}

void dump_stack(u64 *stack) {}

void exc6_handler(Registers *regs) {
  outb(0x20, 0x20);
  kprintf("Exception occured\n");
  kprintf("Invalid opcode.\n");
  asm("sti");
  for (;;)
    ;
}

void irq0_handler(Registers *regs) {
  tick();

  outb(0x20, 0x20); /* EOI */
  schedule(regs);
}

void irq1_handler() {
  /* keyboard driver */
  handle_scan(inb(0x60));

  outb(0x20, 0x20); /* EOI */
}

void irq2_handler() {
  kprintf("IRQ 2 fired!\n");
  outb(0x20, 0x20); /* EOI */
}

void irq3_handler() {
  kprintf("IRQ 3 fired!\n");
  outb(0x20, 0x20); /* EOI */
}

void irq4_handler() {
  kprintf("IRQ 4 fired!\n");
  outb(0x20, 0x20); /* EOI */
}

void irq5_handler() {
  kprintf("IRQ 5 fired!\n");
  outb(0x20, 0x20); /* EOI */
}

void irq6_handler() {
  kprintf("IRQ 6 fired!\n");
  outb(0x20, 0x20); /* EOI */
}

void irq7_handler() {
  kprintf("IRQ 7 fired!\n");
  outb(0x20, 0x20); /* EOI */
}

void irq8_handler() {
  kprintf("IRQ 8 fired!\n");
  outb(0xA0, 0x20);
  outb(0x20, 0x20); /* EOI */
}

void irq9_handler() {
  kprintf("IRQ 9 fired!\n");
  outb(0xA0, 0x20);
  outb(0x20, 0x20); /* EOI */
}

void irq10_handler() {
  kprintf("IRQ 10 fired!\n");
  outb(0xA0, 0x20);
  outb(0x20, 0x20); /* EOI */
}

void irq11_handler() {
  kprintf("IRQ 11 fired!\n");
  outb(0xA0, 0x20);
  outb(0x20, 0x20); /* EOI */
}

void irq12_handler() {
  kprintf("IRQ 12 fired!\n");
  outb(0xA0, 0x20);
  outb(0x20, 0x20); /* EOI */
}

void irq13_handler() {
  kprintf("IRQ 13 fired!\n");
  outb(0xA0, 0x20);
  outb(0x20, 0x20); /* EOI */
}

void irq14_handler() {
  kprintf("IRQ 14 fired!\n");
  outb(0xA0, 0x20);
  outb(0x20, 0x20); /* EOI */
}

void irq15_handler() {
  kprintf("IRQ 15 fired!\n");
  outb(0xA0, 0x20);
  outb(0x20, 0x20); /* EOI */
}

void dummy_handler() {
  // kprintf("Exception occured\n");

  outb(0xA0, 0x20);
  outb(0x20, 0x20); /* EOI */
  __asm__ volatile("cli; hlt");
  for (;;)
    ;
}

void idt_init() {

  // kprintf("Initializing IDT\n");

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
  extern int dummy_irq();

  u64 irq0_addr;
  u64 irq1_addr;
  u64 irq2_addr;
  u64 irq3_addr;
  u64 irq4_addr;
  u64 irq5_addr;
  u64 irq6_addr;
  u64 irq7_addr;
  u64 irq8_addr;
  u64 irq9_addr;
  u64 irq10_addr;
  u64 irq11_addr;
  u64 irq12_addr;
  u64 irq13_addr;
  u64 irq14_addr;
  u64 irq15_addr;
  u64 dummy_irq_addr;

  IDTPtr idt_ptr;

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
  irq1_addr = (unsigned long)irq1;
  irq2_addr = (unsigned long)irq2;
  irq3_addr = (unsigned long)irq3;
  irq4_addr = (unsigned long)irq4;
  irq5_addr = (unsigned long)irq5;
  irq6_addr = (unsigned long)irq6;
  irq7_addr = (unsigned long)irq7;
  irq8_addr = (unsigned long)irq8;
  irq9_addr = (unsigned long)irq9;
  irq10_addr = (unsigned long)irq10;
  irq11_addr = (unsigned long)irq11;
  irq12_addr = (unsigned long)irq12;
  irq13_addr = (unsigned long)irq13;
  irq14_addr = (unsigned long)irq14;
  irq15_addr = (unsigned long)irq15;
  dummy_irq_addr = (unsigned long)dummy_irq;

  for (int i = 0; i < 32; i++)
    idt_set_descriptor(i, (uint64_t)dummy_irq_addr, 0x8e);

  idt_set_descriptor(6, exc6_handler, 0x8e);

  idt_set_descriptor(32, irq0_addr, 0x8e);
  idt_set_descriptor(33, irq1_addr, 0x8e);
  idt_set_descriptor(34, irq2_addr, 0x8e);
  idt_set_descriptor(35, irq3_addr, 0x8e);
  idt_set_descriptor(36, irq4_addr, 0x8e);
  idt_set_descriptor(37, irq5_addr, 0x8e);
  idt_set_descriptor(38, irq6_addr, 0x8e);
  idt_set_descriptor(39, irq7_addr, 0x8e);
  idt_set_descriptor(40, irq8_addr, 0x8e);
  idt_set_descriptor(41, irq9_addr, 0x8e);
  idt_set_descriptor(42, irq10_addr, 0x8e);
  idt_set_descriptor(43, irq11_addr, 0x8e);
  idt_set_descriptor(44, irq12_addr, 0x8e);
  idt_set_descriptor(45, irq13_addr, 0x8e);
  idt_set_descriptor(46, irq14_addr, 0x8e);
  idt_set_descriptor(47, irq15_addr, 0x8e);

  /* fill the IDT descriptor */
  idt_ptr.base = (u64)&idt[0];
  idt_ptr.limit = (u16)(sizeof(IDTEntry) * 256) - 1;

  kprintf("[IDT]  IDT address: 0x%x\n", &idt[0]);

  __asm__ volatile("lidt %0" ::"memory"(idt_ptr));
  //__asm__ volatile ("sti");

  kprintf("[IDT]  Initialized IDT!\n");

  return;
}
