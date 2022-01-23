#!/bin/bash

gcc -nostdlib ../src/kprintf.o  ../src/util.o ../src/cpu/io.o ../src/drivers/serial.o ../src/sfring/strlen.o  -v test-32.c -o a.out

gcc -nostdlib ../src/kprintf.o  ../src/util.o ../src/cpu/io.o ../src/drivers/serial.o ../src/string/strlen.o  -v test.c
