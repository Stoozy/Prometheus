#include <stdio.h>
#include <sysdeps/syscalls.h>

int _putchar(char c) {
  sys_write(0, &c, 1);
  return 0;
}
