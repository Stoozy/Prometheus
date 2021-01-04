#!/bin/bash

. ./iso.sh

qemu-system-x86_64 -no-reboot -monitor stdio -d int -no-shutdown -drive file=fs.tar -cdrom zos.iso

