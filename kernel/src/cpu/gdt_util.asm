global load_gdt
global gdt_flush
global load_tss


load_gdt:
    lgdt [rdi]      ; load GDT, rdi (1st argument) contains the gdt_ptr

    mov ax, 0x30    ; TSS segment is 0x30
    ltr ax          ; load TSS


    mov ax, 0x10    ; kernel data segment is 0x10
    mov ds, ax      ; load kernel data segment in data segment registers
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    pop rdi         ; pop the return address
    mov rax, 0x08   ; kernel code segment is 0x08
    push rax        ; push the kernel code segment
    push rdi        ; push the return address again

    o64 retf        ; do a far return, like a normal return but
                    ; pop an extra argument of the stack
                    ; and load it into CS
    
