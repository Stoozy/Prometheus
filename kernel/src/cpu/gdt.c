#include "gdt.h"
#include "../string/string.h"
#include "../kprintf.h"
#include "../memory/pmm.h"

extern void load_gdt(struct table_ptr *);

struct {
    uint32_t reserved0; uint64_t rsp0;      uint64_t rsp1;
    uint64_t rsp2;      uint64_t reserved1; uint64_t ist1;
    uint64_t ist2;      uint64_t ist3;      uint64_t ist4;
    uint64_t ist5;      uint64_t ist6;      uint64_t ist7;
    uint64_t reserved2; uint16_t reserved3; uint16_t iopb_offset;
} Tss;

__attribute__((aligned(4096)))
volatile struct {
  struct gdt_entry null;
  struct gdt_entry kernel_code;
  struct gdt_entry kernel_data;
  struct gdt_entry null2;
  struct gdt_entry user_data;
  struct gdt_entry user_code;
  struct gdt_entry tss_low;
  struct gdt_entry tss_high;
} gdt_table = {
    {0, 0, 0, 0x00, 0x00, 0},  /* 0x00 null  */
    {0, 0, 0, 0x9a, 0xa0, 0},  /* 0x08 kernel code (kernel base selector) */
    {0, 0, 0, 0x92, 0xa0, 0},  /* 0x10 kernel data */
    {0, 0, 0, 0x00, 0x00, 0},  /* 0x18 null (user base selector) */
    {0, 0, 0, 0x92, 0xa0, 0},  /* 0x20 user data */
    {0, 0, 0, 0x9a, 0xa0, 0},  /* 0x28 user code */
    {0, 0, 0, 0x89, 0xa0, 0},  /* 0x30 tss low */
    {0, 0, 0, 0x00, 0x00, 0},  /* 0x38 tss high */
};

extern volatile u8 user_stack[4096];
extern volatile u8 stack[4096];
void gdt_init(){

    memset((void*)&Tss, 0,  sizeof(Tss));
    uint64_t tss_base = ((uint64_t)&Tss);

    Tss.rsp0 = (uint64_t)&stack[4095] & 0x0000FFFFFFFF;

    //Tss.rsp1 = (uint64_t)pmm_alloc_block()+0x1000;
    //Tss.rsp2 =   (uint64_t)&user_stack[4095] & 0x0000FFFFFFFF;
    
    Tss.iopb_offset = sizeof(Tss);

    //Tss.ist1 = (uint64_t)(pmm_alloc_block() + 0x1000) & 0x0000FFFFFFFF; 
    Tss.ist1 = (uint64_t)&stack[4095] & 0x0000FFFFFFFF;
    //Tss.ist2 = (uint64_t)(pmm_alloc_block() + 0x1000) & 0x0000FFFFFFFF; 
    //Tss.ist3 = (uint64_t)(pmm_alloc_block() + 0x1000) & 0x0000FFFFFFFF; 
    //Tss.ist4 = (uint64_t)(pmm_alloc_block() + 0x1000) & 0x0000FFFFFFFF; 
    //Tss.ist5 = (uint64_t)(pmm_alloc_block() + 0x1000) & 0x0000FFFFFFFF; 
    //Tss.ist6 = (uint64_t)(pmm_alloc_block() + 0x1000) & 0x0000FFFFFFFF; 
    //Tss.ist7 = (uint64_t)(pmm_alloc_block() + 0x1000) & 0x0000FFFFFFFF; 

    gdt_table.tss_low.base15_0 = tss_base & 0xffff;
    gdt_table.tss_low.base23_16 = (tss_base >> 16) & 0xff;
    gdt_table.tss_low.base31_24 = (tss_base >> 24) & 0xff;
    gdt_table.tss_low.limit15_0 = sizeof(Tss);
    gdt_table.tss_high.limit15_0 = (tss_base >> 32) & 0xffff;
    gdt_table.tss_high.base15_0 = (tss_base >> 48) & 0xffff;

    struct table_ptr gdt_ptr = { sizeof(gdt_table)-1, (u64)&gdt_table };

    kprintf("[GDT] GDT Address: 0x%x\n", &gdt_table);
    kprintf("[GDT] GDT Pointer size: %lu bytes\n", sizeof(gdt_ptr));

    load_gdt(&gdt_ptr);
}
