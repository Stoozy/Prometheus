#include <memory/slab.h>
#include <libk/kprintf.h>
#include <libk/ringbuffer.h>
#include <string/string.h>

void rb_init(RingBuffer *rb, size_t cap, size_t tsize) {
  rb->head = rb->tail = rb->len = 0;

  rb->tsize = tsize;
  rb->cap = cap;

  rb->buffer = kmem_alloc(cap * tsize);
}

void rb_destroy(RingBuffer *rb) {
  // TODO: needs a proper kernel allocator
}

int rb_push(RingBuffer *rb, void *item) {
  if (rb->len == rb->cap)
    return 0; // Buffer reached full capacity

  if (rb->head == rb->cap)
    rb->head = 0;

  memcpy(rb->buffer + (rb->head * rb->tsize), item, rb->tsize);
  rb->head++;
  rb->len++;

  return 1;
}

int rb_pop(RingBuffer *rb, void *item) {
  if (!rb || !item)
    return 0;

  if (rb->len == 0)
    return 0;

  memcpy(item, rb->buffer + (rb->tail * rb->tsize), rb->tsize);

  if (rb->tail == rb->cap)
    rb->tail = 0;

  rb->tail++;
  rb->len--;

  return 1;
}
