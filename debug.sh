#!/bin/sh
set -e
#. ./iso.sh

#qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom myos.iso
qemu-system-x86_64 -s -S -drive file=root -cdrom zos.iso & disown
gdb

