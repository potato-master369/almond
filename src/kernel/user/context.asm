; kernel side saving for scheduler

.global context_push_state
context_push_state:
    pushad
    push ds
    push es
    push fs
    push gs
    xor eax, eax
    ret

context_pull_state:
    pop gs
    pop fs
    pop es
    pop ds
    popad
    xor eax, eax
    iretd
