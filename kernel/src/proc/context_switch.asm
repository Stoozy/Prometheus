
;  switch_to_task(void * new_stack, u64 cr3) 

global switch_to_process
global switch_to_user_proc 

; switch_to_user_proc( void * instruction_ptr, void * stack, void * cr3)

switch_to_user_proc:

    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ;push 0x23       ; new stack selector
    ;push rsi        ; stack ptr
    ;push 0x202      ; flags with IF enabled

    ;push 0x2b       ; user code selector
    ;push rdi        ; new instruction pointer

    ;iretq
    
    mov rcx, rdi        ; first argument, new instruction pointer
    mov rsp, rsi        ; load sp
    mov r11, 0x202      ; rflags
    o64 sysret          ; to space!

switch_to_process:
    ; Load new callee-saved registers
    mov rsp, rdi

    ; check if new cr3 needs to be set
    
    mov rcx, cr3
    cmp rcx, rsi
    je .done
    
    ; set new cr3
    mov cr3, rsi

.done:
    pop rdi    
    pop rsi    
    pop rbx    
    pop rbp    
    iretq
