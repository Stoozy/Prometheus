#include "memory/vmm.h"
#include <cpu/idt.h>

#include <libk/kprintf.h>
#include <libk/typedefs.h>

#include <drivers/pit.h>
#include <drivers/serial.h>

#include <cpu/cpu.h>
#include <cpu/io.h>
#include <drivers/keyboard.h>
#include <proc/proc.h>

struct stackframe {
  struct stackframe *rbp;
  uint64_t rip;
};

void stacktrace(struct stackframe *stk, unsigned int num_frames) {
  kprintf("Stacktrace:\n");
  for (unsigned int frame = 0; stk && frame < num_frames; ++frame) {
    // Unwind to previous stack frame
    kprintf("  0x%llx     \n", stk->rip);
    stk = stk->rbp;
  }
}

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

void err0_handler(Registers *regs) {
  kprintf("EXCEPTION: Divide-by-zero Error #DE\n");
  dump_regs(regs);
  for (;;)
    ;
}

void err1_handler(Registers *regs) {
  kprintf("EXCEPTION: Debug #DE\n");
  dump_regs(regs);
  for (;;)
    ;
}

void err2_handler(Registers *regs) {
  kprintf("EXCEPTION: Debug #DE\n");
  dump_regs(regs);
  for (;;)
    ;
}

void err3_handler(Registers *regs) {
  kprintf("EXCEPTION: Breakpoint #BP\n");
  dump_regs(regs);
  for (;;)
    ;
}

void err4_handler(Registers *regs) {
  kprintf("EXCEPTION: Overflow #OF\n");
  dump_regs(regs);
  for (;;)
    ;
}

void err5_handler(Registers *regs) {
  kprintf("EXCEPTION: Bound Range Exceeded #BR\n");
  dump_regs(regs);
  for (;;)
    ;
}

void err6_handler(Registers *regs) {
  kprintf("EXCEPTION: Invalid Opcode #UD\n");
  dump_regs(regs);
  for (;;)
    ;
}

void err7_handler(Registers *regs) {
  kprintf("EXCEPTION: Device not Available #NM\n");
  dump_regs(regs);
  for (;;)
    ;
}

void err8_handler(Registers *regs) {
  kprintf("EXCEPTION: Double Fault  #DF\n");
  dump_regs(regs);
  for (;;)
    ;
}

void err9_handler(Registers *regs) {
  kprintf("EXCEPTION: Coprocessor Segment overrun\n");
  dump_regs(regs);
  for (;;)
    ;
}

void err10_handler(Registers *regs) {
  kprintf("EXCEPTION: Invalid TSS #TS\n");
  dump_regs(regs);
  for (;;)
    ;
}

void err11_handler(Registers *regs) {
  kprintf("EXCEPTION: Segment not present #TS\n");
  dump_regs(regs);
  for (;;)
    ;
}

void err12_handler(Registers *regs) {
  kprintf("EXCEPTION: Stack-Segment Fault #SS\n");
  dump_regs(regs);
  for (;;)
    ;
}

void err13_handler(Registers *regs, int error_code) {
  dump_regs(regs);
  kprintf("Error code: %d\n", error_code);
  kprintf("EXCEPTION: General Protection Fault #GP\n");
  for (;;)
    ;
}

void err14_handler(Registers *regs, int error_code) {

  kprintf("\nEXCEPTION: Page Fault #PF\n");
  extern ProcessControlBlock *running;
  kprintf("Currently running process: %s (pid %d) kstack at 0x%x (base: %x)\n",
          running->name, running->pid, running->kstack,
          running->kstack - STACK_SIZE);

  kprintf("Error code: %d\n", error_code);

  kprintf(error_code & PAGE_PRESENT ? "Page protection violation\n"
                                    : "Page not present\n");
  kprintf(error_code & PAGE_WRITE ? "Caused by write access\n"
                                  : "Caused by read access\n");
  kprintf(error_code & PAGE_USER ? "User mode #PF\n" : "Kernel mode #PF\n");

  uintptr_t addr;
  asm("mov %%cr2, %0" : "=r"(addr)::);

  kprintf("Faulting address: 0x%x\n", addr);

  dump_regs(regs);
  stacktrace((struct stackframe *)regs->rbp, 5);

  for (;;)
    ;
}

void irq0_handler(Registers *regs) {
  tick();

  outb(0x20, 0x20); /* EOI */
  extern u64 g_ticks;
  if (g_ticks % RR_QUANTUM == 0)
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

void idt_init() {

  extern void err0();
  extern void err1();
  extern void err2();
  extern void err3();
  extern void err4();
  extern void err5();
  extern void err6();
  extern void err7();
  extern void err8();
  extern void err9();
  extern void err10();
  extern void err11();
  extern void err12();
  extern void err13();
  extern void err14();
  extern void err15();

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

  irq0_addr = (uint64_t)irq0;
  irq1_addr = (uint64_t)irq1;
  irq2_addr = (uint64_t)irq2;
  irq3_addr = (uint64_t)irq3;
  irq4_addr = (uint64_t)irq4;
  irq5_addr = (uint64_t)irq5;
  irq6_addr = (uint64_t)irq6;
  irq7_addr = (uint64_t)irq7;
  irq8_addr = (uint64_t)irq8;
  irq9_addr = (uint64_t)irq9;
  irq10_addr = (uint64_t)irq10;
  irq11_addr = (uint64_t)irq11;
  irq12_addr = (uint64_t)irq12;
  irq13_addr = (uint64_t)irq13;
  irq14_addr = (uint64_t)irq14;
  irq15_addr = (uint64_t)irq15;

  idt_set_descriptor(0, (uint64_t)err0, 0x8e);
  idt_set_descriptor(1, (uint64_t)err1, 0x8e);
  idt_set_descriptor(2, (uint64_t)err2, 0x8e);
  idt_set_descriptor(3, (uint64_t)err3, 0x8e);
  idt_set_descriptor(4, (uint64_t)err4, 0x8e);
  idt_set_descriptor(5, (uint64_t)err5, 0x8e);
  idt_set_descriptor(6, (uint64_t)err6, 0x8e);
  idt_set_descriptor(7, (uint64_t)err7, 0x8e);
  idt_set_descriptor(8, (uint64_t)err8, 0x8e);
  idt_set_descriptor(9, (uint64_t)err9, 0x8e);
  idt_set_descriptor(10, (uint64_t)err10, 0x8e);
  idt_set_descriptor(11, (uint64_t)err11, 0x8e);
  idt_set_descriptor(12, (uint64_t)err12, 0x8e);
  idt_set_descriptor(13, (uint64_t)err13, 0x8e);
  idt_set_descriptor(14, (uint64_t)err14, 0x8e);

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
  kprintf("[IDT]  Initialized IDT!\n");

  return;
}

void Sleep(u32 ms) {
  extern u64 g_ticks;
  u64 et = g_ticks + ms;

  while (g_ticks != et) {
  };

  return;
}

void enable_irq() { asm("sti"); }
void disable_irq() { asm("cli"); }
