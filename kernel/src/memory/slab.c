#include "drivers/tty.h"
#include "fs/tmpfs.h"
#include "libk/kprintf.h"
#include "memory/vmm.h"
#include <config.h>
#include <libk/util.h>
#include <memory/pmm.h>
#include <memory/slab.h>
#include <proc/proc.h>
#include <string/string.h>
#include <sys/queue.h>

static struct kmem_cache caches[MAX_KMEM_CACHES];

static struct kmem_slab *slab_create(size_t sz) {
  void *addr, *end;
  if (sz < PAGE_SIZE / 8) {
    addr = pmm_alloc_blocks(2) + PAGING_VIRTUAL_OFFSET;
    if (!addr)
      panic("Ran out of physical memory");
    end = addr + (2 * PAGE_SIZE);
  } else {
    addr = pmm_alloc_blocks(8) + PAGING_VIRTUAL_OFFSET;
    if (!addr)
      panic("Ran out of physical memory");
    end = addr + (8 * PAGE_SIZE);
  }

  struct kmem_slab *slab = (struct kmem_slab *)(addr);
  size_t offset = ALIGN_UP(sizeof(struct kmem_slab), sz);

  kprintf("Calculated offset for %d byte size to be %d\n", sz, offset);
  slab->free = addr + offset;

  kprintf("First free is 0x%p\n", slab->free);

  uintptr_t *fp = slab->free;
  while ((void *)fp + sz < end) {
    *fp = (uintptr_t)((void *)fp + sz);
    fp = (uintptr_t *)*fp;
  }
  *fp = 0;

  slab->page = slab;
  slab->refcnt = 0;
  slab->boundary = end;

  return slab;
}

struct kmem_cache kmem_cache_create(size_t sz) {
  if (sz > 2 * PAGE_SIZE)
    panic("slab allocator: requested size is bigger than 8K, use "
          "pmm_alloc_block ");

  struct kmem_cache cache = {.objsize = sz};
  LIST_INIT(&cache.slabs);

  struct kmem_slab *new_slab = slab_create(sz);
  LIST_INSERT_HEAD(&cache.slabs, new_slab, entries);

  return cache;
}

static int cache_idx_from_size(size_t sz) {
  for (int i = 0; i < MAX_KMEM_CACHES; i++)
    if (caches[i].objsize >= sz) // assume sorted
      return i;

  return -1;
}

static void dump_slab(struct kmem_slab *slab) {
  void **fp = slab->free;
  while (fp) {
    kprintf("object @ 0x%x; next is @ 0x%x\n", fp, *fp);
    if (fp == *fp)
      panic("corrupt freelist in slab \n");
    fp = *fp;
  }
}

void *kmem_alloc(size_t sz) {
  int cache_idx = cache_idx_from_size(sz);

  if (cache_idx == -1) {
    // size is most likely too large
    int num_pages = DIV_ROUND_UP(sz, PAGE_SIZE) + 1;
    void *addr = pmm_alloc_blocks(num_pages);
    *(size_t *)addr = num_pages;
    return addr + PAGE_SIZE;
  }

  struct kmem_cache cache = caches[cache_idx];

  if (cache.objsize < sz)
    panic("allocating from a smaller sized cache");

  struct kmem_slab *slab;
  LIST_FOREACH(slab, &cache.slabs, entries) {
    if (slab->free != NULL) {
      break;
    }
  }

  if (slab == NULL || slab->free == NULL) {
    kmem_cache_grow(&caches[cache_idx]);
    return kmem_alloc(sz);
  }

  void *ret = slab->free;
  // kprintf("Current free is 0x%p\n", slab->free);
  uintptr_t next = *(uintptr_t *)slab->free;
  // kprintf("Next is %x\n", next);
  slab->free = *(void **)slab->free;

  return ret;
}

static struct kmem_slab *slab_from_ptr(void *ptr) {
  for (int i = 0; i < MAX_KMEM_CACHES; i++) {
    // TODO: only checking one slab for now
    struct kmem_slab *slab;
    LIST_FOREACH(slab, &caches[i].slabs, entries) {
      if (ptr >= slab->page && ptr < slab->boundary)
        return slab;
    }
  }

  return NULL;
}

void kmem_free(void *ptr) {
  kprintf("Freeing 0x%p", ptr);
  struct kmem_slab *slab = slab_from_ptr(ptr);

  if (!slab) {
    // panic("Trying to free memory from an invalid slab");
    kprintf("Freeing memory allocated from pmm");
    size_t *blocks = ptr - PAGE_SIZE;
    if (*blocks == 0)
      panic("Freeing 0 blocks...");

    pmm_free_blocks((uintptr_t)ptr - PAGE_SIZE, *blocks);
    return;
  }

  // update free pointer to current free object
  *(uintptr_t *)(ptr) = (uintptr_t)slab->free;
  // update current free object to the newly freed object
  slab->free = ptr;
}

void kmem_cache_free(struct kmem_cache *cp, void *buf) {}

void kmem_cache_grow(struct kmem_cache *cp) {
  struct kmem_slab *new_slab;
  new_slab = slab_create(cp->objsize);
  LIST_INSERT_HEAD(&cp->slabs, new_slab, entries);
}

void kmem_cache_reap(struct kmem_cache *cp, void *buf) {}

void kmem_init() {

  memset(&caches[0], 0, MAX_KMEM_CACHES * sizeof(struct kmem_cache));
  // powers of 2
  caches[0] = kmem_cache_create(8);
  caches[1] = kmem_cache_create(16);
  caches[2] = kmem_cache_create(32);
  caches[3] = kmem_cache_create(64);
  caches[4] = kmem_cache_create(96);
  caches[5] = kmem_cache_create(128);
  caches[6] = kmem_cache_create(256);
  caches[7] = kmem_cache_create(512);
  caches[8] = kmem_cache_create(1024);

  // kernel structures
  caches[9] = kmem_cache_create(sizeof(struct tty));
  caches[10] = kmem_cache_create(sizeof(struct process_control_block));
}
