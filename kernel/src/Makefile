KERNEL := dead_kernel.elf

CC=gcc
NASM=nasm 
AS=as

CFLAGS = -g -Wall -Wextra -O2 -pipe 

INTERNALLDFLAGS := \
	-Wl,-static,--no-dynamic-linker,-ztext \
	-fno-pic  -fPIE \
	-nostdlib      \
	-Tlinker.ld    \
	-z max-page-size=0x1000 \
	-mcmodel=large -v

INTERNALCFLAGS  :=       \
	-I.                  \
	-Wall				 \
	-Wextra				 \
	-std=gnu11           \
	-ffreestanding       \
	-fno-stack-protector \
	-fno-pic -fPIE       \
	-mno-80387           \
	-mno-mmx             \
	-mno-3dnow           \
	-mno-sse             \
	-mno-sse2            \
	-mno-red-zone		\
	-mcmodel=large

CFILES := $(shell find ./ -type f -name '*.c')
ASM_FILES := $(shell find ./ -type f -name '*.asm')
AS_FILES := $(shell find ./ -type f -name '*.S')
C_OBJ    := $(CFILES:.c=.o)
NASM_OBJ    := $(ASM_FILES:.asm=.o)
.PHONY: all clean

all: $(KERNEL)

$(KERNEL): $(NASM_OBJ) $(C_OBJ) $(AS_OBJ)
	$(CC) $(INTERNALLDFLAGS)  $(NASM_OBJ) $(C_OBJ) -o $@

stivale.h:
	wget https://github.com/stivale/stivale/raw/master/stivale.h

%.o : %.asm
	$(NASM) -f elf64 $< -o $@

%.o: %.c stivale.h 
	$(CC) $(CFLAGS) $(INTERNALCFLAGS) -c $< -o $@

clean:
	rm -rf $(KERNEL) $(C_OBJ) $(NASM_OBJ)
