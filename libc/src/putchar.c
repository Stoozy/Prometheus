
void putchar(char c) {
  register int syscall asm("rsi") = 4; // SYS_WRITE

  register int file asm("r8") = 0;
  register char *ptr asm("r9") = &c;
  register int len asm("r10") = 1;

  register int bytes_written asm("r15");
  asm("syscall"
      : "=r"(bytes_written)
      : "r"(file), "r"(ptr), "r"(len)
      : "rcx", "r11");

  return;
}
