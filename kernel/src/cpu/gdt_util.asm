global load_gdt
global gdt_flush


gdt_flush:
    lgdt [rdi]

;gdt_flush:
;    lgdt [rdi]
;    push rbp
;    mov rbp, rsp
;
;    ;push qword 0x10
;    push 0x10
;    push rbp
;    pushf
;    ;push qword 0x8
;    push 0x8
;    push .trampoline
;    iretq
;
;.trampoline:
;    pop rbp
;
;    mov ax, 0x10
;
;    mov ds, ax
;    mov es, ax
;    mov fs, ax
;    mov gs, ax
;    mov ss, ax
;
;    mov ax, 0x28
;    ltr ax
;
;    ret

;load_gdt:
;    cli
;    lgdt [rdi]      ; load GDT, rdi (1st argument) contains the gdt_ptr
;
;    mov ax, 0x28    ; TSS segment is 0x28
;    ltr ax          ; load TSS
;
;    mov ax, 0x10    ; kernel data segment is 0x10
;    mov ds, ax      ; load kernel data segment in data segment registers
;    mov es, ax
;    mov fs, ax
;    mov gs, ax
;    mov ss, ax
;
;    pop rdi         ; pop the return address
;    mov rax, 0x08   ; kernel code segment is 0x08
;    push rax        ; push the kernel code segment
;    push rdi        ; push the return address again
;
;    o64 retf           ; do a far return, like a normal return but
                    ; pop an extra argument of the stack
                    ; and load it into CS
