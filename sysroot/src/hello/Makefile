CC=gcc
INC_DIRS= ../mlibc/options/ansi/include \
		  ../mlibc/options/bsd/include \
		  ../mlibc/options/elf/include \
		  ../mlibc/options/posix/include \
		  ../mlibc/options/internal/include \
		  ../mlibc/sysdeps/atlas/include	\
		  ../mlibc/build/	\

LIBDIR=$(SYSROOT)/lib
INCLUDEDIR=$(SYSROOT)/include
BINDIR=$(SYSROOT)/bin

# mlibc
CFLAGS= -fPIC -shared -no-pie -nostartfiles -nostdlib

# Custom libc
#
#CFLAGS= -O0 -fPIC -static -nostartfiles -nostdlib

.PHONY: all clean 
all: hello 

hello:
	$(CC) $(CFLAGS) hello_world.c -I$(INCLUDEDIR) -L$(LIBDIR) $(LIBDIR)/crt0.o $(LIBDIR)/crti.o -lc -o $(BINDIR)/hello


clean:
	rm -rf $(BINDIR)/hello

