#!/bin/bash

mkdir $HOME/src
SRC_DIR=$HOME/src

export PREFIX="$HOME/opt/cross"
export TARGET="i386-elf"
export PATH="$PREFIX/bin:$PATH"

cd $SRC_DIR 
# Download and Extract binutils
wget https://ftp.gnu.org/gnu/binutils/binutils-2.31.tar.gz 
tar -xvzf  binutils-2.31.tar.gz 
rm -rf binutils-2.31.tar.gz

# Configure and Install binutils
mkdir build-binutils 
cd build-binutils 
linux32 ../binutils-2.31/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror 
linux32 make 
linux32 make install

cd $SRC_DIR
# Download and Extract gcc
wget https://ftp.gnu.org/gnu/gcc/gcc-11.1.0/gcc-11.1.0.tar.gz 
tar -xvzf gcc-11.1.0.tar.gz 
rm -rf gcc-11.1.0.tar.gz

# Configure and Install gcc
mkdir build-gcc 
cd build-gcc 
linux32 ../gcc-11.1.0/configure  --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers 
linux32 make all-gcc 
linux32 make all-target-libgcc 
linux32 make install-gcc 
linux32 make install-target-libgcc


