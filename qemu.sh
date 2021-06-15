#!/bin/bash

. ./iso.sh

qemu-system-x86_64 -cpu qemu32,+pae -no-reboot -monitor stdio -d int -no-shutdown -m 4G -cdrom dead_os.iso  -drive id=disk,file=drive.img\
