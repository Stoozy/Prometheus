#ifndef PIT_H
#define PIT_H 1

#include "../typedefs.h"

void pit_init(u32 hz);
void sleep(u32 ms);
void tick();

#endif
