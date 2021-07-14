[bits 64]

global load_idt

global irq0
global irq1
global irq2
global irq3
global irq4
global irq5
global irq6
global irq7
global irq8
global irq9
global irq10
global irq11
global irq12
global irq13
global irq14
global irq15

extern kprintf

section .data

success_msg db `Initialized IDT!\n`, 0
irq_fired_msg db `Interrupt fired!\n`, 0

section .text

irq0:
    pushfq
    jmp print_irq_fired
    ;call irq0_handler
    popfq
    iretq
 
irq1:
    pushfq
    jmp print_irq_fired
    ;call irq1_handler
    popfq
    iretq
 
irq2:
    pushfq
    jmp print_irq_fired
    ;call irq2_handler
    popfq
    iretq
 
irq3:
    pushfq
    jmp print_irq_fired
    ;call irq3_handler
    popfq
    iretq
 
irq4:
    pushfq
    jmp print_irq_fired
    ;call irq4_handler
    popfq
    iretq
 
irq5:
    pushfq
    jmp print_irq_fired
    ;call irq5_handler
    popfq
    iretq
 
irq6:
    pushfq
    jmp print_irq_fired
    ;call irq6_handler
    popfq
    iretq
 
irq7:
    pushfq
    jmp print_irq_fired
    ;call irq7_handler
    popfq
    iretq
 
irq8:
    pushfq
    jmp print_irq_fired
    ;call irq8_handler
    popfq
    iretq
 
irq9:
    pushfq
    jmp print_irq_fired
    ;call irq9_handler
    popfq
    iretq
 
irq10:
    pushfq
    jmp print_irq_fired
    ;call irq10_handler
    popfq
    iretq
 
irq11:
    pushfq
    jmp print_irq_fired
    ;call irq11_handler
    popfq
    iretq
 
irq12:
    pushfq
    jmp print_irq_fired
    ;call irq12_handler
    popfq
    iretq
 
irq13:
    pushfq
    jmp print_irq_fired
    ;call irq13_handler
    popfq
    iretq
 
irq14:
    pushfq
    jmp print_irq_fired
    ;call irq14_handler
    popfq
    iretq
 
irq15:
    pushfq
    jmp print_irq_fired
    ;call irq15_handler
    popfq
    iretq

load_idt:
    mov edx, [esp+4] 
    lidt [edx]
    sti

    push rbp
    lea rdi, [rel success_msg]
    call kprintf  wrt  ..plt
    pop rbp

    ret

print_irq_fired:
    push rbp
    lea rdi, [rel irq_fired_msg]
    call kprintf wrt ..plt
    pop rbp
    ret

    
