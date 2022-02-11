CURRDIR=$(pwd)
 
# make symlinks (a bad hack) to make newlib work
cd ~/opt/cross/bin/ # this is where the bootstrapped generic cross compiler toolchain (i686-elf-xxx) is installed in,
                # change this based on your development environment.
#cd /usr/bin
ln x86_64-unknown-elf-ar x86_64-atlas-ar
ln x86_64-unknown-elf-as x86_64-atlas-as
ln x86_64-unknown-elf-gcc x86_64-atlas-gcc
ln x86_64-unknown-elf-gcc x86_64-atlas-cc
ln x86_64-unknown-elf-ranlib x86_64-atlas-ranlib
 
# return
cd $CURRDIR
