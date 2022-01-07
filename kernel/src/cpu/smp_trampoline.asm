global ap_entry


[bits 16]
ap_entry:
    cli
    ;cld

    ;mov ax, aprunning
    ;inc ax

loop:
    int 0x10
    jmp loop
