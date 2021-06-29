#!/bin/bash
. ./headers.sh

ROOT_DIR=$PWD
cd kernel/arch/i386/ && sh make-nasm.sh

cd $ROOT_DIR
for PROJECT in $PROJECTS; do
  (cd $PROJECT && DESTDIR="$SYSROOT" $MAKE install -j4)
done






