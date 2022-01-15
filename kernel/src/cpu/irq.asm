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
global dummy_irq 

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

hang:
    hlt
    jmp hang

dummy_irq:
	pushaq
	call dummy_handler 
    jmp hang
	;popaq
	;iretq
 
irq0:
	pushaq
	call irq0_handler
	popaq
	iretq
    
irq1:
    pushfq
    call irq1_handler
    popfq
    iretq
 
irq2:
    pushfq
    call irq2_handler
    popfq
    iretq
 
irq3:
    pushfq
    call irq3_handler
    popfq
    iretq
 
irq4:
    pushfq
    call irq4_handler
    popfq
    iretq
 
irq5:
    pushfq
    call irq5_handler
    popfq
    iretq
 
irq6:
    pushfq
    call irq6_handler
    popfq
    iretq
 
irq7:
    pushfq
    call irq7_handler
    popfq
    iretq
 
irq8:
    pushfq
    call irq8_handler
    popfq
    iretq
 
irq9:
    pushfq
    call irq9_handler
    popfq
    iretq
 
irq10:
    pushfq
    call irq10_handler
    popfq
    iretq
 
irq11:
    pushfq
    call irq11_handler
    popfq
    iretq
 
irq12:
    pushfq
    call irq12_handler
    popfq
    iretq
 
irq13:
    pushfq
    call irq13_handler
    popfq
    iretq
 
irq14:
    pushfq
    call irq14_handler
    popfq
    iretq
 
irq15:
    pushfq
    call irq15_handler
    popfq
    iretq


