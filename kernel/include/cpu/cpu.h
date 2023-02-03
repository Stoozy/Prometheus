#pragma once

#include <libk/typedefs.h>
#include <memory/vmm.h>

#define EFER 0xC0000080

#define STAR 0xC0000081
#define LSTAR 0xC0000082
#define CSTAR 0xC0000083
#define SFMASK 0xC0000084

#define FSBASE 0xC0000100
#define GSBASE 0xC0000101
#define KGSBASE 0xC0000102

typedef struct {
  u64 rdi;
  u64 rsi;
  u64 rax;
  u64 rbx;
  u64 rcx;
  u64 rdx;

  u64 rbp;

  u64 r15;
  u64 r14;
  u64 r13;
  u64 r12;
  u64 r11;
  u64 r10;
  u64 r9;
  u64 r8;

  u64 rip;
  u64 cs;
  u64 rflags;
  u64 rsp;
  u64 ss;
} __attribute__((packed)) Registers;

typedef struct {
  u64 *syscall_kernel_stack; // 0x0
  u64 *syscall_user_stack;   // 0x8
  PageTable *kcr3;           // 0x10
  PageTable *pcr3;           // 0x18

  Registers * regs;
} __attribute__((packed)) LocalCpuData;

void cpu_init(u8);
LocalCpuData *get_cpu_struct(u8);
void dump_regs(Registers *);


static inline uint64_t rdmsr(uint64_t msr) {
  uint32_t low, high;
  asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
  return ((uint64_t)high << 32) | low;
}

static inline void wrmsr(uint64_t msr, uint64_t value) {
  uint32_t low = value & 0xFFFFFFFF;
  uint32_t high = value >> 32;
  asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}
