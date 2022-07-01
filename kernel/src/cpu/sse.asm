
section .text

global sse_init

sse_init:

  mov rax, cr0
  and rax, ~(1<<2)
  or  rax, 0x2
  mov cr0, rax

  mov rax, cr4
  or  rax, 3 << 9
  mov cr4, rax
  ret

