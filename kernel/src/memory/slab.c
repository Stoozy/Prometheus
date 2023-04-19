#include "drivers/tty.h"
#include "fs/tmpfs.h"
#include "libk/kprintf.h"
#include "memory/vmm.h"
#include <config.h>
#include <libk/util.h>
#include <memory/pmm.h>
#include <memory/slab.h>
#include <proc/proc.h>

static struct kmem_cache caches[MAX_KMEM_CACHES];

static struct kmem_slab *slab_create(size_t sz) {
  void *addr, *end;
  if (sz < PAGE_SIZE / 8) {
    addr = pmm_alloc_blocks(3);
    if (!addr)
      panic("Ran out of physical memory");
    end = addr + (3 * PAGE_SIZE);
  } else {
    addr = pmm_alloc_blocks(9);

    if (!addr)
      panic("Ran out of physical memory");
    end = addr + (9 * PAGE_SIZE);
  }

  struct kmem_slab *slab = (struct kmem_slab *)(addr);

  size_t offset = sz < PAGE_SIZE ? PAGE_SIZE % sz : sz % PAGE_SIZE;
  void *start = (addr + PAGE_SIZE) + offset; // first page is for metadata only

  uintptr_t *fp = start;
  while ((void *)fp + sz < end) {
    *fp = (uintptr_t)((void *)fp + sz);
    fp = (uintptr_t *)*fp;
  }
  *fp = 0;

  slab->free = slab->page = start;
  slab->next = slab->prev = NULL;

  slab->refcnt = 0;
  slab->boundary = end;

  return slab;
}

struct kmem_cache kmem_cache_create(size_t sz) {
  if (sz > 2 * PAGE_SIZE)
    panic("slab allocator: requested size is bigger than 8K, use "
          "pmm_alloc_block ");

  struct kmem_cache new = {.objsize = sz};
  new.slabs = slab_create(sz);

  return new;
}

static int cache_idx_from_size(size_t sz) {
  for (int i = 0; i < MAX_KMEM_CACHES; i++)
    if (caches[i].objsize >= sz)
      return i;

  return -1;
}

static void dump_slab(struct kmem_slab *slab) {
  uintptr_t *fp = slab->free;
  while (fp) {
    kprintf("object @ 0x%x; next is @ %x\n", fp, *fp);
    fp = (uintptr_t *)*fp;
  }
}

void *kmem_alloc(size_t sz) {
  int cache_idx = cache_idx_from_size(sz);

  if (cache_idx == -1) {
    char msg[250];
    sprintf(msg, "No slab for size %x\n", sz);
    panic(msg);
  }

  struct kmem_cache cache = caches[cache_idx];
  struct kmem_slab *slab = cache.slabs;

  if (slab->free) {
    void *ret = slab->free;
    slab->free += sz;
    return ret;
  }

  // TODO: make new slabs once a slab has run out
  panic("Slab ran out of chunks");
  return NULL;
}

static struct kmem_slab *slab_from_ptr(void *ptr) {
  for (int i = 0; i < MAX_KMEM_CACHES; i++) {
    // TODO: only checking one slab for now
    if (ptr >= caches[i].slabs->page && ptr < caches[i].slabs->boundary)
      return caches[i].slabs;
  }

  return NULL;
}

void kmem_free(void *ptr) {
  struct kmem_slab *slab = slab_from_ptr(ptr);
  if (!slab)
    panic("Trying to free memory from an invalid slab");

  kprintf("Before freeing %x\n", ptr);
  dump_slab(slab);

  // update free pointer to current free object
  *(uintptr_t *)(ptr) = (uintptr_t)slab->free;

  // update current free object to the newly freed object
  slab->free = ptr;

  kprintf("After freeing\n");
  dump_slab(slab);
}

void kmem_cache_free(struct kmem_cache *cp, void *buf) {}

void kmem_init() {

  // powers of 2
  caches[0] = kmem_cache_create(8);
  caches[1] = kmem_cache_create(16);
  caches[2] = kmem_cache_create(32);
  caches[3] = kmem_cache_create(64);
  caches[4] = kmem_cache_create(128);
  caches[5] = kmem_cache_create(256);
  caches[6] = kmem_cache_create(512);
  caches[7] = kmem_cache_create(1024);

  // kernel structures
  caches[8] = kmem_cache_create(sizeof(struct process_control_block));
  caches[9] = kmem_cache_create(sizeof(struct tty));
  caches[10] = kmem_cache_create(sizeof(struct tmpnode));
}
