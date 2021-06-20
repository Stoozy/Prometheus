#!/bin/sh
set -e
. ./headers.sh

cd kernel/arch/i386/ && sh make-nasm.sh

cd ~/Downloads/Dead-OS/
for PROJECT in $PROJECTS; do
  (cd $PROJECT && DESTDIR="$SYSROOT" $MAKE install -j4)
done






