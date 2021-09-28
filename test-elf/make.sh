#!/bin/bash

gcc -nostdlib ../src/kprintf.o  ../src/util.o ../src/cpu/io.o ../src/drivers/serial.o ../src/string/strlen.o  -v test.c
