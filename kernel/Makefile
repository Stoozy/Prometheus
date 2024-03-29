KERNEL := kernel.elf
INCLUDEDIR=$(shell pwd)/include

CC=gcc
NASM=nasm  
AS=as

CFLAGS = -g -Wall -Wextra -O0 -pipe 
NASMFLAGS = -f elf64 -I$(INCLUDEDIR)

INTERNALLDFLAGS := \
	-Wl,-static,--no-dynamic-linker,-ztext \
	-fno-pic  -fPIE \
	-nostdlib      \
	-Tlinker.ld    \
	-z max-page-size=0x1000 \
	-mcmodel=large -v 

INTERNALCFLAGS  :=       \
	-I$(INCLUDEDIR)       			 \
	-Wall				 \
	-Wextra				 \
	-std=gnu11           \
	-ffreestanding       \
	-fno-stack-protector \
	-fno-pic -fPIE       \
	-mno-80387           \
	-mno-mmx             \
	-mno-3dnow           \
	-mno-red-zone		 \
	-mcmodel=large		\
	#-mno-sse             \
	#-mno-sse2            \
	

CFILES := $(shell find ./ -type f -name '*.c')
ASM_FILES := $(shell find ./ -type f -name '*.asm')
AS_FILES := $(shell find ./ -type f -name '*.S')
C_OBJ    := $(CFILES:.c=.o)
NASM_OBJ    := $(ASM_FILES:.asm=.o)
.PHONY: all clean

all: $(KERNEL)

$(KERNEL): $(NASM_OBJ) $(C_OBJ) $(AS_OBJ)
	$(CC) $(INTERNALLDFLAGS) $(NASM_OBJ) $(C_OBJ)  -o $@

stivale2.h:
	wget https://github.com/stivale/stivale/raw/master/stivale2.h

%.o : %.asm
	$(NASM) $(NASMFLAGS) $< -o $@

%.o: %.c stivale2.h 
	$(CC)  $(CFLAGS) $(INTERNALCFLAGS) -c $< -o $@

clean:
	rm -rf $(KERNEL) $(C_OBJ) $(NASM_OBJ)
