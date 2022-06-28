CC=gcc
INC_DIRS= ../mlibc/options/ansi/include \
		  ../mlibc/options/bsd/include \
		  ../mlibc/options/elf/include \
		  ../mlibc/options/posix/include \
		  ../mlibc/options/internal/include \
		  ../mlibc/sysdeps/atlas/include	\
		  ../mlibc/build/	\
INC_PARAMS=$(foreach d, $(INC_DIRS), -I$d)
LIBRARIES=$(shell pwd)/lib


CFLAGS= -fPIC -shared -no-pie -nostartfiles -LD_LIBRARY_PATH=lib/

.PHONY: all clean 
all: hello 

hello:
	$(CC) $(CFLAGS) hello_world.c $(INC_PARAMS) -L$(LIBRARIES) lib/crt1.o -lc -o hello
	patchelf hello --set-interpreter /lib/ld.so

clean:
	rm -rf hello


