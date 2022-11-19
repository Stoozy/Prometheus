# Osdev

A hobby operating system for the x86\_64 arch aiming to use a hybrid kernel.

## Getting Started

### Tools

* gcc and friends (build-essential on ubuntu based distros)
* qemu
* xorriso
* git
* xbstrap
* meson


### Installing

To run this OS:

* make sure you have the tools listed above
* Clone the repo
* make a `build` dir and cd `build`
* run `xbstrap install bash` (this will also install all prerequisite packages)
* inside project rootdir, run `yes | cp -r build/system-root/* sysroot/`
* then run `mkdir sysroot/lib && cp -r sysroot/usr/lib/* sysroot/lib/`
* `make run` will start up the os in qemu

## Current functionality

* Mapping pages
* Basic round robin scheduler
* Pre-emptive multitasking
* Userspace Programs
* Loading elf binaries

## License

This project is licensed under the MIT License - see the LICENSE file for details

