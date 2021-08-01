#ifndef PORT_IO
#define PORT_IO

#include <typedefs.h>

void outb(u16 port, u8 val);
u8 inb(u16 port);

#endif
