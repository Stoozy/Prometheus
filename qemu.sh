#!/bin/bash

. ./iso.sh

qemu-system-i386  -vga std -machine q35 -cpu qemu32,+pae -no-reboot -monitor stdio -d int -no-shutdown -m 4G  -cdrom dead_os.iso  
    -drive id=disk,file=dead.img,if=none \
    #-device ahci,id=ahci \
    #-device ide-drive,drive=disk,bus=ahci.0
