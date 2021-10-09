#!/bin/bash

CURDIR=$(PWD)

cd ~/cross/bin/

ln x86_64-elf-ar x86_64-deados-ar
ln x86_64-elf-as x86_64-deados-as
ln x86_64-elf-gcc x86_64-deados-gcc
ln x86_64-elf-gcc x86_64-deados-cc
ln x86_64-elf-ranlib x86_64-deados-ranlib

cd $CURDIR
