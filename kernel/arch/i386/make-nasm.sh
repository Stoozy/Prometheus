#!/bin/bash

nasm -f elf irq.asm -o irq.o
nasm -f elf gdt.asm -o gdt-as.o
nasm -f elf paging-as.asm -o paging-as.o
