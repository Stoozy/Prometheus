; Entry for syscalls

global syscall_entry

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
    push rbx
    push rsi
    push rdi
%endmacro

%macro popaq	0
    pop rdi    
    pop rsi    
    pop rbx    
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


; call number in rdi, args start at r8 and end at r15
syscall_entry:
    cli
    swapgs
    mov [gs:0x8], rsp       ; save process stack
    mov rsp, [gs:0x0]       ; switch to syscall stack

    push qword 0x30         ; user ss
    push qword [gs:0x8]     ; saved rsp
    push r11                ; rflags
    push qword 0x2b         ; user cs
    push rcx                ; rip


    pushaq

    mov rdi, rsp
    mov rbp, 0
    call syscall_dispatcher

    popaq

    mov rsp, [gs:0x8]
    swapgs
    sti

    o64 sysret

    ;mov ax, 0x30
    ;mov ds, ax
	;mov es, ax 
	;mov fs, ax 
	;mov gs, ax ; SS is handled by iret

    ;mov rax, rsp
    ;push 0x30
    ;push rax
    ;push 0x202
    ;push 0x2b
    ;push rcx

    ;iretq

    ;popaq
    ;mov r11, 0x202

    ;o64 sysret



