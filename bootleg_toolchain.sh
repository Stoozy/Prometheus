CURRDIR=$(pwd)
 
# make symlinks (a bad hack) to make newlib work
#cd ~/cross/bin/ # this is where the bootstrapped generic cross compiler toolchain (i686-elf-xxx) is installed in,
                # change this based on your development environment.
cd /usr/bin
ln ar x86_64-atlas-ar
ln as x86_64-atlas-as
ln gcc x86_64-atlas-gcc
ln gcc x86_64-atlas-cc
ln ranlib x86_64-atlas-ranlib
 
# return
cd $CURRDIR
