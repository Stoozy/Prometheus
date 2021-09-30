
;  switch_to_task(void * new_stack, u64 cr3) 

global switch_to_process

%macro popaq	0
    pop rdi    
    pop rsi    
    pop rbx    
    pop rbp    
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
