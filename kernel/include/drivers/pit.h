#ifndef PIT_H
#define PIT_H 1

#include <typedefs.h>

void pit_init(u32 hz);
void Sleep(u32 ms);
u64 tick();

#endif
