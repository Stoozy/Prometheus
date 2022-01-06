global ap_entry
extern ap_startup

[bits 16]
ap_entry:

    cli
    cld
    ljmp    0x0:0x8040

_L8010_GDT_table:
    dl 0
    dl 0
    dl 0x0000FFFF, 0x00CF9A00    ; flat code
    dl 0x0000FFFF, 0x008F9200    ; flat data
    dl 0x00000068, 0x00CF8900    ; tss
    align 16
_L8030_GDT_value:
    dw _L8030_GDT_value - _L8010_GDT_table - 1
    dl 0x8010
    dl 0, 0
    align 64
_L8040:
    xorw  ax,ax
    mov ds, ax
    lgdt 0x8030
    mov eax, cr0
    orl eax, 1
    mov cr0, eax
    jmp 0x08:0x8060

    ;xorw    %ax, %ax
    ;movw    %ax, %ds
    ;lgdtl   0x8030
    ;movl    %cr0, %eax
    ;orl     $1, %eax
    ;movl    %eax, %cr0
    ;ljmp    $8, $0x8060
    align 32

[bits 32]
_L8060:
    mov ax, 16
    mov ds, ax
    mov ss, ax

    mov eax, 1
    cpuid
    shrl ebx, 24
    mov edi, ebx

    shll ebx, 15
    mov esp, stack_top 
    sub esp, ebx
    push edi

wait:  pause
    cmp bspdone, 0
    jz wait
    
    ljmp 0x08:ap_startup
    
    ;movw    $16, %ax
    ;movw    %ax, %ds
    ;movw    %ax, %ss
    ;; get our Local APIC ID
    ;mov     $1, %eax
    ;cpuid
    ;shrl    $24, %ebx
    ;movl    %ebx, %edi
    ; set up 32k stack, one for each core. It is important that all core must have its own stack
    ;shll    $15, %ebx
    ;movl    stack_top, %esp
    ;subl    %ebx, %esp
    ;pushl   %edi
    ; spinlock, wait for the BSP to finish
;1:  pause
;    cmpb    $0, bspdone
;    jz      1b
;    lock    incb aprunning
;    ; jump into C code (should never return)
;    ;ljmp    $8, $ap_startup
;    jmp 0x08:[ap_startup]

