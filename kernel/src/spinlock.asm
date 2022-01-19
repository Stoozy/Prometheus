global acquire_lock
global release_lock

extern print_spin_wait

acquire_lock:
    lock bts dword [rdi], 0
    jc .spin_wait
    ret

.spin_wait:
    call print_spin_wait
    test dword [rdi], 1
    jnz .spin_wait

release_lock:
    mov dword [rdi], 0
    ret

