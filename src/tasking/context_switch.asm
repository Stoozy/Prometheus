
;  switch_to_task(ProcessControlBlock ** old, ProcessControlBlock * new) 

global switch_to_process


switch_to_process:
    mov rax, [  rdi  ]
    mov rbx, [  rsi  ]
    
    ; save registers
    push rbp
    push rbx
    push rsi
    push rdi

    ; switch stacks
    mov rsp, [rax]  ; rax = pointer to rsp pointer
    mov rdx, rsp 


    ; Load new callee-saved registers
    pop rdi
    pop rsi
    pop rbx
    pop rbp
    ret
