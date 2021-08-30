; switch(struct context **old, struct context *new);

global context_switch
global switch_to_task 

switch_to_task:

	mov rsp, rdi

	pop rdi
	pop rsi
	pop rbp
	pop rbx

	ret

context_switch:

	mov rax, [rsp+8]
	mov rdx, [rsp+16]

	; Save old caller-saved registers
	push rbp
	push rbx
	push rsi
	push rdi

	; Switch stacks
	mov [rax], rsp
	mov rsp, rdx

	; Load new callee-saved registers
	pop rdi
	pop rsi
	pop rbx
	pop rbp
	ret
