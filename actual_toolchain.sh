!#/bin/bash

CURRDIR=$(pwd)

export PREFIX=$CURDIR/toolchain &&
export TARGET=x86_64-atlas &&
export PATH=$PREFIX/bin:$PATH &&

wget https://ftp.gnu.org/gnu/binutils/binutils-2.24.tar.gz &&
tar -xvf binutils-2.24.tar.gz &&

rm -rf binutils-2.24.tar.gz  && 

patch -p0 binutils-2.24/bfd/config.bfd binutils_bfd.patch &&
patch -p0 binutils-2.24/config.sub binutils_config.patch &&
patch -p0 binutils-2.24/gas/configure.tgt binutils_gas.patch &&
patch -p0 binutils-2.24/ld/configure.tgt binutils_ld.patch &&
cp elf_i386_atlas.sh binutils-2.24/ld/emulparams/ &&
cp elf_x86_64_atlas.sh binutils-2.24/ld/emulparams/ &&
patch -p0 binutils-2.24/ld/Makefile.am binutils_ld_makefile.patch &&
cd binutils-2.24/ld/ && automake && cd $CURDIR

#mkdir build-binutils &&
#cd build-binutils &&
#../binutils-2.24/configure --target=$TARGET --prefix=$PREFIX --with-sysroot --disable-nle --disable-werror &&
#make && 
#make install && 
#
#rm -rf build-binutils && 
#
#
#which -- $TARGET-as || echo $TARGET-as is not in the PATH && 
#
#cd $CURDIR

#wget https://ftp.gnu.org/gnu/gcc/gcc-11.2.0/gcc-11.2.0.tar.gz && 
#tar -xvf gcc-11.2.0.tar.gz &&
#
#mkdir build-gcc &&
#cd build-gcc &&
#
#../gcc-11.2.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers --with-sysroot=$CURDIR/sysroot --with-newlib  &&
#
#make all-gcc &&
#make all-target-libgcc &&
#make install-gcc &&
#make install-target-libgcc &&
#
#cd $CURDIR &&
#rm -rf build-gcc &&
#rm -rf gcc-11.2.0.tar.gz 


