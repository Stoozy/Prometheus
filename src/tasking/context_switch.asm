
;  switch_to_task(void * new_stack) 

global switch_to_process

%macro popaq	0
    pop rdi    
    pop rsi    
    pop rbx    
    pop rbp    
%endmacro


switch_to_process:
    ;mov rax, [  rdi  ]
    ;mov rbx, [  rsi  ]
    
    ; save registers
    ;push rbp
    ;push rbx
    ;push rsi
    ;push rdi

    ; switch stacks
    ;mov rsp, [rax]  ; rax = pointer to rsp pointer
    ;mov rsp, rdx

    ; Load new callee-saved registers

    mov rsp, rdi

    popaq

    iretq
