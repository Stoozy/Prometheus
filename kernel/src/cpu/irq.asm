%include "cpu/macros.mac"

global load_idt

global err0 
global err1 
global err2 
global err3 
global err4
global err5
global err6
global err7
global err8
global err9
global err10
global err11
global err12
global err13
global err14


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

extern err0_handler
extern err1_handler
extern err2_handler
extern err3_handler
extern err4_handler
extern err5_handler
extern err6_handler
extern err7_handler
extern err8_handler
extern err9_handler
extern err10_handler
extern err11_handler
extern err12_handler
extern err13_handler
extern err14_handler


extern irq0_handler
extern irq1_handler
extern irq2_handler
extern irq3_handler
extern irq4_handler
extern irq5_handler
extern irq6_handler
extern irq7_handler
extern irq8_handler
extern irq9_handler
extern irq10_handler
extern irq11_handler
extern irq12_handler
extern irq13_handler
extern irq14_handler
extern irq15_handler
extern dummy_handler 
extern scheduler
extern tick 

section .text


err0:
    pushaq
    mov rdi, rsp
    call err0_handler
    popaq
 
err1:
    pushaq
    mov rdi, rsp
    call err1_handler
    popaq
 
err2:
    pushaq
    mov rdi, rsp
    call err2_handler
    popaq
 
 
err3:
    pushaq
    mov rdi, rsp
    call err2_handler
    popaq
 
err4:
    pushaq
    mov rdi, rsp
    call err4_handler
    popaq

err5:
    pushaq
    mov rdi, rsp
    call err5_handler
    popaq


err6:
    pushaq
    mov rdi, rsp
    call err6_handler
    popaq

err7:
    pushaq
    mov rdi, rsp
    call err7_handler
    popaq

err8:
    pushaq
    mov rdi, rsp
    call err8_handler
    popaq

err9:
    pushaq
    mov rdi, rsp
    call err9_handler
    popaq

err10:
    pushaq
    mov rdi, rsp
    call err10_handler
    popaq

err11:
    pushaq
    mov rdi, rsp
    call err11_handler
    popaq

err12:
    pushaq
    mov rdi, rsp
    call err12_handler
    popaq


err13:
    mov dword rsi, [rsp]
    add rsp, 8
    pushaq
    mov rdi, rsp
    call err13_handler
    popaq

err14:
    mov dword rsi, [rsp]
    add rsp, 8
    pushaq
    mov rdi, rsp
    call err14_handler
    popaq


irq0:

    pushaq
    mov rdi, rsp
    call irq0_handler

    popaq
    iretq

irq1:
    pushaq
    call irq1_handler
    popaq
    iretq
 
irq2:
    pushaq
    call irq2_handler
    popaq
    iretq
 
irq3:
    pushaq
    call irq3_handler
    popaq
    iretq
 
irq4:
    pushaq
    call irq4_handler
    popaq
    iretq
 
irq5:
    pushaq
    call irq5_handler
    popaq
    iretq
 
irq6:
    pushaq
    call irq6_handler
    popaq
    iretq
 
irq7:
    pushaq
    call irq7_handler
    popaq
    iretq
 
irq8:
    pushaq
    call irq8_handler
    popaq
    iretq

extern schedule
irq9:
    pushaq
    mov rdi, rsp
    call schedule

    popaq
    iretq

irq10:
    pushaq
    call irq10_handler
    popaq
    iretq
 
irq11:
    pushaq
    call irq11_handler
    popaq
    iretq
 
irq12:
    pushaq
    call irq12_handler
    popaq
    iretq
 
irq13:
    pushaq
    call irq13_handler
    popaq
    iretq
 
irq14:
    pushaq
    call irq14_handler
    popaq
    iretq
 
irq15:
    pushaq
    call irq15_handler
    popaq
    iretq



