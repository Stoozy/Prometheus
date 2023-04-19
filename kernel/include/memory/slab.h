
#include <unistd.h>

#pragma once

#include <stdint.h>

struct kmem_slab;

struct kmem_slab {
  void *free;
  void *page;
  void *boundary;

  int refcnt;

  struct kmem_slab *next;
  struct kmem_slab *prev;
};

struct kmem_cache {
  struct kmem_slab *slabs;
  size_t objsize;
};

struct kmem_cache kmem_cache_create(size_t sz);
void kmem_cache_free(struct kmem_cache *cp, void *buf);

void *kmem_alloc(size_t sz);
void kmem_free(void *ptr);

void kmem_init();
