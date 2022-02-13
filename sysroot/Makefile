CC=x86_64-atlas-gcc
INCLUDE=$(shell pwd)/usr/x86_64-atlas/include
LIBRARIES=$(shell pwd)/usr/x86_64-atlas/lib
CRT0=$(shell pwd)/usr/x86_64-atlas/lib/crt0.o
LIBC=$(shell pwd)/usr/x86_64-atlas/lib/libc.a

CFLAGS=-nostartfiles  --freestanding -nostdlib

#x86_64-atlas-gcc -I/home/stoozy/Dead-OS/sysroot/usr/x86_64-atlas/include -L/home/stoozy/Dead-OS/sysroot/usr/x86_64-atlas/lib/ -lc ./usr/x86_64-atlas/lib/crt0.o exit.c -nostartfiles

all: hello 


hello:
	$(CC) $(CFLAGS) hello_world.c -I$(INCLUDE) -L$(LIBRARIES) $(LIBC) $(CRT0) -o hello

clean:
	rm -rf hello


