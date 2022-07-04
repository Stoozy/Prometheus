; Entry for syscalls

global syscall_entry
global enable_sce

extern syscall_dispatcher

%macro pushaq 	0
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    push rbp

    push rdx
    push rcx
    push rbx
    push rax
    push rsi
    push rdi
%endmacro

%macro popaq	0
    pop rdi    
    pop rsi    
    pop rax
    pop rbx    
    pop rcx    
    pop rdx

    pop rbp    

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
%endmacro



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

; call number in rdi
; arg1  r8
; arg2  r9
; arg3  r10
; arg4  r12
; arg5  r13
; arg6  r14
; ret   r15

syscall_entry:
    cli

    swapgs

    mov [gs:0x8], rsp       ; save process stack
    mov rdi, cr3            ; save process cr3
    mov [gs:0x18], rdi


    mov rsp, [gs:0x0]       ; switch to syscall stack

    push qword 0x23         ; user ss
    push qword [gs:0x8]     ; saved rsp
    push qword r11          ; rflags
    push qword 0x2b         ; user cs
    push qword rcx          ; rip

    pushaq
    cld

    mov rdi, rsp
    mov rbp, 0
    call syscall_dispatcher

    popaq
    mov rsp, [gs:0x8]       ; back to user stack

    mov rbx, [gs:0x18]      ; back to process cr3
    mov cr3, rbx      

    swapgs

    o64 sysret

