CC=gcc
INC_DIRS= ../mlibc/options/ansi/include \
		  ../mlibc/options/bsd/include \
		  ../mlibc/options/elf/include \
		  ../mlibc/options/posix/include \
		  ../mlibc/options/internal/include \
		  ../mlibc/sysdeps/atlas/include	\
		  ../mlibc/build/	\
INC_PARAMS=$(foreach d, $(INC_DIRS), -I$d)

BINDIR= $(SYSROOT)/bin
LIBDIR= $(SYSROOT)/lib

BINARY=$(BINDIR)/hello


CFLAGS= -fPIC -shared -no-pie -nostartfiles -nostdlib -LD_LIBRARY_PATH=$(LIBDIR)

.PHONY: all clean 
all:  $(BINARY)

$(BINARY):
	$(CC) $(CFLAGS) hello_world.c $(INC_PARAMS) -L$(LIBDIR) $(LIBDIR)/crt0.o -lc -o $@
	patchelf $(BINARY) --set-interpreter /lib/ld.so

clean:
	rm -rf $(BINARY)


