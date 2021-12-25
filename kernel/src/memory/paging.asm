global load_pagedir;
global invalidate_tlb;

load_pagedir:
    mov cr3, rdi
    retq


invalidate_tlb:
    mov rax, cr3
    mov cr3, rax
    retq
