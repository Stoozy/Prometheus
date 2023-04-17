#include "abi-bits/fcntl.h"
#include <proc/elf.h>
#include <proc/proc.h>

#include <abi-bits/auxv.h>
#include <config.h>
#include <fs/vfs.h>
#include <libk/kmalloc.h>
#include <libk/kprintf.h>
#include <libk/typedefs.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <proc/proc.h>
#include <stdint.h>
#include <string/string.h>

extern void load_pagedir(PageTable *);

#define LD_BASE 0xA000000

u8 validate_elf(u8 *elf) {
  if (elf[0] != 0x7f || elf[1] != 'E' || elf[2] != 'L' || elf[3] != 'F')
    return 0; /* invalid elf header */

  return 1;
}

Auxval load_elf_segments(ProcessControlBlock *proc, u8 *elf_data) {

  PageTable *vas = proc->cr3;
  kprintf("[ELF]  Load elf segments called with %x vas and %x elf buffer\n",
          vas, elf_data);
  Auxval aux = {0};

  // load elf file here
  Elf64_Ehdr *elf_hdr = (Elf64_Ehdr *)elf_data;
  aux.entry = elf_hdr->e_entry;
  aux.phnum = elf_hdr->e_phnum;
  aux.phent = elf_hdr->e_phentsize;

  for (u64 segment = 0; segment < elf_hdr->e_phnum; ++segment) {
    Elf64_Phdr *p_header = (Elf64_Phdr *)(elf_data + elf_hdr->e_phoff +
                                          (elf_hdr->e_phentsize * segment));

    if (p_header->p_type == PT_PHDR) {
      kprintf("PT_PHDR vaddr is 0x%x\n", p_header->p_vaddr);
      aux.phdr = (u64)p_header->p_vaddr;
    }

    if (p_header->p_type == PT_INTERP) {
      char *ld_path = kmalloc(p_header->p_memsz);
      memset(ld_path, 0, p_header->p_memsz);
      memcpy(ld_path, (elf_data + p_header->p_offset), p_header->p_filesz);

      kprintf("[ELF]  Got interpreter file path: %s\n", ld_path);
      File *ld_file = vfs_open(ld_path, 0);
      u8 *ld_data = kmalloc(ld_file->vn->size);
      int br = vfs_read(ld_file, ld_data, ld_file->vn->size);

      if (!(br != 0 && validate_elf(ld_data)))
        continue;

      Elf64_Ehdr *ld_hdr = (Elf64_Ehdr *)ld_data;
      aux.ld_entry = (LD_BASE + ld_hdr->e_entry);

      for (u64 lds = 0; lds < ld_hdr->e_phnum; ++lds) {
        Elf64_Phdr *ldph = (Elf64_Phdr *)(ld_data + ld_hdr->e_phoff +
                                          (ld_hdr->e_phentsize * lds));

        if (ldph->p_type != PT_LOAD)
          continue;

        u64 offset = ldph->p_vaddr & (PAGE_SIZE - 1);
        u64 blocks = (ldph->p_memsz / PAGE_SIZE) + 2;

        void *paddr = pmm_alloc_blocks(blocks) + PAGING_VIRTUAL_OFFSET;
        void *vaddr = (void *)(LD_BASE + (ldph->p_vaddr & ~(0xfff)));

        memset(paddr, 0, ldph->p_memsz);
        memcpy(paddr + offset, (ld_data + ldph->p_offset), ldph->p_filesz);

        int page_flags = PAGE_USER | PAGE_WRITE | PAGE_PRESENT;
        vmm_map_range(vas, vaddr, paddr - PAGING_VIRTUAL_OFFSET,
                      blocks * PAGE_SIZE, page_flags);

        VASRangeNode *range = kmalloc(sizeof(VASRangeNode));

        range->virt_start = vaddr;
        range->phys_start = paddr - PAGING_VIRTUAL_OFFSET;
        range->size = blocks * PAGE_SIZE;
        range->page_flags = page_flags;
        range->next = NULL;

        proc_add_vas_range(proc, range);
      }
    }

    if (p_header->p_type != PT_LOAD)
      continue;

    u64 offset = p_header->p_vaddr & (PAGE_SIZE - 1);
    u64 blocks = (p_header->p_memsz / PAGE_SIZE) + 1;

    void *phys_addr = pmm_alloc_blocks(blocks);
    void *virt_addr = (void *)(p_header->p_vaddr - offset);

    memset(phys_addr, 0, p_header->p_memsz);
    memcpy(phys_addr + offset, (elf_data + p_header->p_offset),
           p_header->p_filesz);

    int page_flags = PAGE_USER | PAGE_WRITE | PAGE_PRESENT;
    vmm_map_range(vas, virt_addr, phys_addr, blocks * PAGE_SIZE, page_flags);

    VASRangeNode *range = kmalloc(sizeof(VASRangeNode));

    range->virt_start = virt_addr;
    range->phys_start = phys_addr;
    range->size = blocks * PAGE_SIZE;
    range->page_flags = page_flags;
    range->next = NULL;

    proc_add_vas_range(proc, range);
  }

  return aux;
}

ProcessControlBlock *create_elf_process(const char *path, char *argvp[],
                                        char *envp[]) {

  File *elf_file = vfs_open(path, 0);
  if (!elf_file) {
    kprintf("[ELF] Coudln't find %s in any mounted filesystem\n", path);
    for (;;)
      ;
  }

  uint8_t *elf_data = kmalloc(elf_file->vn->size);

  int br = vfs_read(elf_file, elf_data, elf_file->vn->size);

  kprintf("Read data %s\n", elf_data);
  if (!validate_elf(elf_data))
    return NULL;

  ProcessControlBlock *proc = (kmalloc(sizeof(ProcessControlBlock)));
  memset(((void *)proc), 0, sizeof(ProcessControlBlock));

  memcpy(proc->name, path, 256);

  void *stack_ptr = pmm_alloc_blocks(STACK_BLOCKS) + (STACK_SIZE);
  void *stack_base = stack_ptr - (STACK_SIZE);

  kprintf("Process stack at 0x%x\n", stack_ptr);

  proc->vas = NULL;
  proc->cr3 = vmm_create_user_proc_pml4(proc); // just maps kernel and returns

  int pflags = PAGE_USER | PAGE_WRITE | PAGE_PRESENT;
  vmm_map_range(proc->cr3, stack_base, stack_base, STACK_SIZE, pflags);

  VASRangeNode *range = kmalloc(sizeof(VASRangeNode));

  range->virt_start = stack_base;
  range->phys_start = stack_base;
  range->size = STACK_SIZE;
  range->page_flags = pflags;
  range->next = NULL;

  proc_add_vas_range(proc, range);

  kprintf("Elf file size is %llu bytes\n", elf_file->vn->size);

  Auxval aux = load_elf_segments(proc, elf_data);

  uint64_t *stack = (uint64_t *)(stack_ptr);

  int envp_len;
  for (envp_len = 0; envp[envp_len] != NULL; envp_len++) {
    size_t length = strlen(envp[envp_len]);
    stack = (void *)stack - length - 1;
    memcpy(stack, envp[envp_len], length);
  }

  int argv_len;
  for (argv_len = 0; argvp[argv_len] != NULL; argv_len++) {
    size_t length = strlen(argvp[argv_len]);
    stack = (void *)stack - length - 1;
    memcpy(stack, argvp[argv_len], length);
  }

  stack = (uint64_t *)(((uintptr_t)stack / 16) * 16);

  if (((argv_len + envp_len + 1) & 1) != 0) {
    stack--;
  }

  *--stack = 0;
  *--stack = 0;
  *--stack = aux.entry;
  *--stack = AT_ENTRY;
  *--stack = aux.phent;
  *--stack = AT_PHENT;
  *--stack = aux.phnum;
  *--stack = AT_PHNUM;
  *--stack = aux.phdr;
  *--stack = AT_PHDR;

  uintptr_t old_rsp = (uintptr_t)stack_ptr;

  *--stack = 0; // end envp
  stack -= envp_len;
  for (int i = 0; i < envp_len; i++) {
    old_rsp -= strlen(envp[i]) + 1;
    stack[i] = old_rsp;
  }

  *--stack = 0; // end argvp
  stack -= argv_len;
  for (int i = 0; i < argv_len; i++) {
    old_rsp -= strlen(argvp[i]) + 1;
    stack[i] = old_rsp;
  }

  *--stack = argv_len; // argc

  stack_ptr -= (stack_ptr - (void *)stack);

  memset(&proc->trapframe, 0, sizeof(Registers));

  proc->trapframe.ss = 0x23;
  proc->trapframe.cs = 0x2b;
  proc->trapframe.rsp = (uintptr_t)stack_ptr;
  proc->trapframe.rflags = 0x202;
  proc->trapframe.rip = aux.ld_entry;

  memset(proc->fd_table, 0, sizeof(uintptr_t) * MAX_PROC_FDS);

  proc->fd_table[0] = vfs_open("/dev/tty", O_RDONLY | O_CREAT);

  if (proc->fd_table[0] == NULL) {
    kprintf("Couldn't open /dev/tty\n");
    for (;;)
      ;
  }
  proc->fd_table[0] = vfs_open("/dev/tty0", O_RDONLY);
  proc->fd_table[1] = vfs_open("/dev/tty0", O_WRONLY);
  proc->fd_table[2] = vfs_open("/dev/tty0", O_WRONLY);

  proc->mmap_base = MMAP_BASE;

  proc->parent = NULL;

  proc->cr3 = (void *)proc->cr3 - PAGING_VIRTUAL_OFFSET;

  proc->kstack = pmm_alloc_blocks(STACK_BLOCKS) + STACK_SIZE;

  extern uint64_t pid_counter;
  proc->pid = pid_counter++;

  kprintf("fd 0 is at %x\n", proc->fd_table[0]);
  kprintf("fd 1 is at %x\n", proc->fd_table[1]);
  kprintf("fd 2 is at %x\n", proc->fd_table[2]);

  proc->cwd = kmalloc(2);
  sprintf(proc->cwd, "/");

  TAILQ_INIT(&proc->children);

  return proc;
}
