CC=gcc
INC_DIRS= ../mlibc/options/ansi/include \
		  ../mlibc/options/bsd/include \
		  ../mlibc/options/elf/include \
		  ../mlibc/options/posix/include \
		  ../mlibc/options/internal/include \
		  ../mlibc/sysdeps/atlas/include	\
		  ../mlibc/build/
INC_PARAMS=$(foreach d, $(INC_DIRS), -I$d)
LIBRARIES=$(shell pwd)/lib


CFLAGS=-nostartfiles -nostdlib -fPIC -no-pie -LD_LIBRARY_PATH=lib/ld.so

.PHONY: all clean 
all: hello 

hello:
	$(CC) $(CFLAGS) hello_world.c $(INC_PARAMS) -L$(LIBRARIES)  crt0.o -lc -o hello
	patchelf hello --set-interpreter a0:lib/ld.so

clean:
	rm -rf hello


