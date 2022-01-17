; C declaration
; switch_to_process(void * new_stack, void* cr3) 

global switch_to_process


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
    popaq
    iretq
