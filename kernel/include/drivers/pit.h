#ifndef PIT_H
#define PIT_H 1

#include <libk/typedefs.h>

void pit_init(u32 hz);
void Sleep(u32 ms);
void tick();

#endif
