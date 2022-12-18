; C declaration
; void switch_to_process(Registers * trapframe, PageTable* cr3);
global switch_to_process

%include "cpu/macros.mac"

switch_to_process:

    mov rcx, cr3 
    cmp rcx, rsi    ; check if new cr3 needs to be set
    je .done

    mov cr3, rsi    ; set new cr3

.done:
    mov rsp, rdi    ; Load new callee-saved registers

    popaq
    iretq
