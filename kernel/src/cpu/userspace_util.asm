; Enabling system call extensions syscall/sysret

global enable_sce
global to_userspace
global set_kernel_entry

enable_sce:
	mov rcx, 0xc0000082
	wrmsr
	mov rcx, 0xc0000080
	rdmsr
	or eax, 1
	wrmsr
	mov rcx, 0xc0000081
	rdmsr
	mov edx, 0x00180008
	wrmsr
    ret

;set_kernel_entry:
    ;wrmsr 0xc0000082, rdi
    ;ret

;to_userspace:
;    mov rcx, rdi        ; first argument, new instruction pointer
;    mov rsp, rsi        ; second argument, new stack pointer
;    mov r11, 0x202      ; rflags
;    o64 sysret          ; to space!
