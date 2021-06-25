#!/bin/bash

. ./iso.sh

qemu-system-i386  -vga std -cpu qemu32,+pae -no-reboot -monitor stdio -d int -no-shutdown -m 4G  -cdrom dead_os.iso  -drive id=disk,file=dead.img\
