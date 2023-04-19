#include "memory/slab.h"
#include <libk/kprintf.h>
#include <memory/pmm.h>
#include <memory/vmm.h>

#include <stddef.h>

void *placement_address = NULL;

void *heap_start = NULL;
void *heap_end = NULL;

// dummy allocator for now

void *kmalloc(size_t size) {
  if (placement_address + size > heap_end) {
    kprintf("Reached end of heap memory\n");
    for (;;)
      ; /* out of memory */
  }

  void *tmp = placement_address;
  placement_address += size;

  return tmp;
}

void kfree(void *ptr) {
  /* TODO */
  kprintf("freeing %x\n", ptr);
  return;
}

void kmalloc_init(size_t heap_size) {
  if (heap_size % PAGE_SIZE != 0) {
    kprintf("Heap size is not page aligned\n");
    for (;;)
      ;
  }

  int blocks = heap_size / PAGE_SIZE;
  placement_address =
      (void *)(PAGING_VIRTUAL_OFFSET + pmm_alloc_blocks(blocks));

  heap_start = placement_address;
  heap_end = placement_address + heap_size;

  return;
}
