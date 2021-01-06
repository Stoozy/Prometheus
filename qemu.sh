#!/bin/bash

. ./iso.sh

qemu-system-x86_64 -no-reboot -monitor stdio -d int -no-shutdown -cdrom zos.iso  -drive id=disk,file=drive.img\
