CC=x86_64-atlas-gcc
INCLUDE=$(shell pwd)/usr/x86_64-atlas/include
LIBRARIES=$(shell pwd)/usr/x86_64-atlas/lib
CRT0=$(shell pwd)/usr/x86_64-atlas/lib/crt0.o
LIBC=$(shell pwd)/usr/x86_64-atlas/lib/libc.a

CFLAGS= -nostdlib -fPIC

all: hello 

hello:
	$(CC) $(CFLAGS) hello_world.c -I$(INCLUDE) -L$(LIBRARIES) crt0.o -o hello

clean:
	rm -rf hello


