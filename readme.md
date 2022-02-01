# Osdev

A hobby operating system for the x86\_64 arch aiming to use a hybrid kernel.

## Getting Started

### Tools

* A 64 bit C compiler such as gcc or clang.
* qemu
* xorriso
* git

### Installing

To run this OS:

* make sure you have the tools listed above
* Clone the repo
* run `make`
* Then, either do `make run` or run qemu yourself with your own flags.

## Current functionality

* Mapping pages
* Basic round robin scheduler
* Pre-emptive multitasking
* Userspace Programs
* Loading elf binaries

## License

This project is licensed under the MIT License - see the LICENSE file for details

