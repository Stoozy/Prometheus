#include <stdio.h>
#include <sysdeps/syscalls.h>

int putchar(int c) { sys_write(0, c, 1); }
