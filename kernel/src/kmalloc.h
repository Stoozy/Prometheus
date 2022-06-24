#pragma once

#include <stddef.h>

void * kmalloc(size_t size);
void kfree(void * ptr);
void kmalloc_init(size_t heap_size);
