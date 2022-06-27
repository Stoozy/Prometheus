LIBC=libc.a
AR=x86_64-atlas-ar
AS=x86_64-atlas-as
CC=x86_64-atlas-cc

CFLAGS= -g -nostartfiles -nostdlib -Wall -Wextra -O0 -pipe -static

CFILES := $(shell find ./ -type f -name '*.c')
NASM_FILES := $(shell find ./ -type f -name '*.asm')
AS_FILES := $(shell find ./ -type f -name '*.S')

CRT0 := crt/crt0.o
CRTI := crt/crti.o
CRTN := crt/crtn.o
C_OBJS := $(CFILES:.c=.o)
NASM_OBJS := $(ASM_FILES:.asm=.o)
AS_OBJS := $(AS_FILES:.S=.o)

INCLUDEDIR:=./include

.PHONY: all clean install

all: $(LIBC) install

$(LIBC): $(NASM_OBJS) $(AS_OBJS) $(C_OBJS)
	$(AR) rcs $@ $(NASM_OBJS) $(C_OBJS)  

%.o: %.asm
	$(NASM) -f elf64 $< -o $@

%.o: %.S
	$(AS) $< -o $@

%.o: %.c
	$(CC)  $(CFLAGS)  -I$(INCLUDEDIR) $(CFLAGS) -c $< -o $@

install:
	cp $(LIBC) $(AS_OBJS) $(SYSROOT)/lib

clean:

	rm -rf $(NASM_OBJS) $(C_OBJS) $(AS_OBJS) $(LIBC)
