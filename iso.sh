#!/bin/sh
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/dead.kernel isodir/boot/dead.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "Dead OS" {
	multiboot /boot/dead.kernel
}
EOF
grub-mkrescue -o dead_os.iso isodir
