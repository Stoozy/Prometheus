global _start

_start:
    mov rsi, 0
    syscall

    mov rax, rbx
    mov rbx, rdx
    mov rax, rdx

.loop:
    jmp .loop
