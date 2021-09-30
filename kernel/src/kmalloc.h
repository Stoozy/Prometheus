#include <stddef.h>
#include "typedefs.h"

#ifndef _KMALLOC_H
#define _KMALLOC_H 1 


void kmalloc_init(u64 mem_size);
void * kmalloc(size_t size);

#endif
