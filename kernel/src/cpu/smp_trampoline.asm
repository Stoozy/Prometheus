global ap_entry


[bits 16]
ap_entry:
    cli
    jmp loop

loop:
    int 0x10
    jmp loop
