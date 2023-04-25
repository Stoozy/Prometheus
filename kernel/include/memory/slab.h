#pragma once

#include <stdint.h>
#include <sys/queue.h>
#include <unistd.h>

struct kmem_slab;

LIST_HEAD(slab_list, kmem_slab);

struct kmem_slab {
  void *free;
  void *page;
  void *boundary;

  int refcnt;

  LIST_ENTRY(kmem_slab) entries;
};

struct kmem_cache {
  struct slab_list slabs;
  size_t objsize;
};

struct kmem_cache kmem_cache_create(size_t sz);
void kmem_cache_free(struct kmem_cache *cp, void *buf);

// backend
void kmem_cache_grow(struct kmem_cache *cp);
void kmem_cache_reap(struct kmem_cache *cp, void *buf);

// frontend
void *kmem_alloc(size_t sz);
void kmem_free(void *ptr);

void kmem_init();
