#ifndef _KMALLOC_H
#define _KMALLOC_H 1 

#include <stddef.h>
#include "typedefs.h"

#define _KMALLOC_ADDRESS_LIMIT       0x2b000
#define _KMALLOC_ADDRESS_BEGIN       0x1

void * kmalloc(size_t size);

#endif
