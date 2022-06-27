#define SYS_EXIT 0
#define SYS_OPEN 1
#define SYS_CLOSE 2
#define SYS_READ 3
#define SYS_WRITE 4
#define SYS_LOG_LIBC 5
#define SYS_VM_MAP 6
#define SYS_SEEK 7
#define SYS_TCB_SET 8

// TODO:
//   actually check for error codes before returning
//   result

int sys_open(const char *file, int flags) {
  register int syscall asm("rsi") = SYS_OPEN;
  register const char *r8 asm("r8") = file;
  register int r9 asm("r9") = flags;

  register int r15 asm("r15");

  asm("syscall" : "=r"(r15) : "r"(r8) : "rcx", "r11", "memory");

  if (r15 > 0)
    return r15;

  return -1;
}

int sys_write(int fd, char *buffer, int len) {
  register int syscall asm("rsi") = SYS_WRITE;

  register int r8 asm("r8") = fd;
  register char *r9 asm("r9") = buffer;
  register int r10 asm("r10") = len;

  register int r15 asm("r15");

  asm("syscall" : "=r"(r15) : "r"(r8) : "rcx", "r11", "memory");

  if (r15 > 0) {
    return r15; // bytes written
  }

  return -1;
}

int exit(int code) {
  register int syscall asm("rsi") = SYS_EXIT;
  register int r8 asm("r8") = code;
  register int r15 asm("r15");

  asm("syscall" : "=r"(r15) : "r"(r8) : "rcx", "r11", "memory");
}
