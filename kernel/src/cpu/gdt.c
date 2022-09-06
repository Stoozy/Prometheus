#include <cpu/gdt.h>
#include <libk/kprintf.h>
#include <memory/pmm.h>
#include <string/string.h>

extern void load_gdt(void *);
extern void load_tss();

struct gdt_entry {
  u16 limit15_0;
  u16 base15_0;
  u8 base23_16;
  u8 type;
  u8 limit19_16_and_flags;
  u8 base31_24;
} __attribute__((packed));

typedef struct {
  u32 reserved0;
  u64 rsp0;
  u64 rsp1;
  u64 rsp2;
  u64 reserved1;
  u64 ist1;
  u64 ist2;
  u64 ist3;
  u64 ist4;
  u64 ist5;
  u64 ist6;
  u64 ist7;
  u64 reserved2;
  u16 reserved3;
  u16 iopb_offset;
} __attribute__((packed)) TSS;

TSS g_tss;

__attribute__((aligned(4096))) typedef struct {

  struct gdt_entry null;
  struct gdt_entry kernel_code;
  struct gdt_entry kernel_data;
  struct gdt_entry null2;
  struct gdt_entry user_data;
  struct gdt_entry user_code;
  struct gdt_entry tss_low;
  struct gdt_entry tss_high;

} GdtTable;

GdtTable gdt_table = {

    {0, 0, 0, 0x00, 0x00, 0}, /* 0x00 null  */
    {0, 0, 0, 0x9a, 0x20, 0}, /* 0x08 kernel code (kernel base selector) */
    {0, 0, 0, 0x92, 0x00, 0}, /* 0x10 kernel data */
    {0, 0, 0, 0x00, 0x00, 0}, /* 0x18 null (user base selector) */
    {0, 0, 0, 0xf2, 0xa0, 0}, /* 0x20 user data */
    {0, 0, 0, 0xfa, 0x20, 0}, /* 0x28 user code */
    {0, 0, 0, 0x89, 0xa0, 0}, /* 0x30 tss low */
    {0, 0, 0, 0x00, 0x00, 0}, /* 0x38 tss high */

};

void gdt_init() {

  TSS *tss = &g_tss;
  u64 tss_base = ((u64)tss);

  memset(tss, 0, sizeof(TSS));

  extern u8 stack[4096];
  tss->rsp0 = (u64)&stack[4095];
  kprintf("[GDT]  TSS stack at 0x%x\n", tss->rsp0);

  gdt_table.tss_low.base15_0 = tss_base & 0xffff;
  gdt_table.tss_low.base23_16 = (tss_base >> 16) & 0xff;
  gdt_table.tss_low.base31_24 = (tss_base >> 24) & 0xff;
  gdt_table.tss_low.limit15_0 = sizeof(TSS);

  gdt_table.tss_high.limit15_0 = (tss_base >> 32) & 0xffff;
  gdt_table.tss_high.base15_0 = (tss_base >> 48) & 0xffff;

  struct table_ptr gdt_ptr = {sizeof(gdt_table) - 1, (u64)&gdt_table};
  load_gdt(&gdt_ptr);
}
