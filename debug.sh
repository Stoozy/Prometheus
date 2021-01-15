#!/bin/sh
set -e
#. ./iso.sh

#qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom myos.iso
qemu-system-x86_64 -no-reboot   -m 4G -no-shutdown -cdrom zos.iso -s -S -drive id=disk,file=drive.img & disown
gdb --ex "target remote localhost:1234"  --ex "set architecture i386:x86-64" --ex "file kernel/zos.kernel"

