; Entry for syscalls

%include "cpu/macros.mac"

global syscall_entry
global enable_sce


extern syscall_dispatcher



enable_sce:
    mov rcx, 0xc0000080     ; EFER MSR
    rdmsr                   ; read current EFER
    or eax, 1               ; enable SCE bit
    wrmsr                   ; write back new EFER
    mov rcx, 0xc0000081     ; STAR MSR
    rdmsr                   ; read current STAR
    mov edx, 0x180008       ; load up GDT segment bases 0x0 (kernel) and 0x18 (user)
    wrmsr                   ; write back new STAR
    ret                     ; return back to C

syscall_entry:

    cli
    swapgs

    mov [gs:0x8], rsp       ; save process stack
    mov rsp, [gs:0x0]       ; switch to syscall stack


    ; pushing registers struct
    push qword 0x23         ; user ss
    push qword [gs:0x8]     ; saved rsp

    swapgs

    push qword r11          ; rflags
    push qword 0x2b         ; user cs
    push qword rcx          ; rip

    pushaq

    mov rdi, rsp
    mov rbp, 0

    call syscall_dispatcher
    cli

    popaq

    pop qword rcx           ; user rip
    add rsp, 8              
    pop qword r11           ; user rflags

    pop qword rsp           ; user rsp
    o64 sysret

