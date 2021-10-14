; Enabling system call extensions syscall/sysret

global enable_sce
global to_userspace

enable_sce:
    mov rcx, 0xc0000080 ; EFER MSR
    rdmsr               ; read current EFER
    or eax, 1           ; enable SCE bit
    wrmsr               ; write back new EFER
    mov rcx, 0xc0000081 ; STAR MSR
    rdmsr               ; read current STAR
    mov edx, 0x00180008 ; load up GDT segment bases 0x0 (kernel) and 0x18 (user)
    wrmsr               ; write back new STAR
    ret                 ; return back to C

to_userspace:
    mov rcx, rdi        ; first argument, new instruction pointer
    mov rsp, rsi        ; second argument, new stack pointer
    mov r11, 0x0202     ; eflags
    o64 sysret;         ; to space!
