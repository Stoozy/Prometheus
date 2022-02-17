extern main

global _start

_start:
    mov rbp, 0
    push rbp
    mov rbp, rsp

    call main

    pop rbp

    ret



