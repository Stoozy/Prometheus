#!/bin/bash

nasm -f elf irq.asm -o irq.o
nasm -f elf gdt.asm -o gdt-as.o
nasm -f elf e820.asm -o e820.o
nasm -f elf paging.asm -o paging.o
