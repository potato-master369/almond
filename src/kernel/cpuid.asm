[bits 32]

global check_cpuid

section .text
; code from OSDev Wiki
; source: https://wiki.osdev.org/CPUID (modified)
check_cpuid:
    pushfd                               ; Save original EFLAGS
    pushfd                               ; Store EFLAGS copy on stack
    xor dword [esp], 0x00200000          ; Invert the ID bit in stored EFLAGS
    popfd                                ; Load stored EFLAGS (with ID bit inverted)
    pushfd                               ; Store EFLAGS again (ID bit may or may not be inverted)
    pop eax                              ; eax = modified EFLAGS
    xor eax, [esp]                       ; eax = whichever bits were changed
    popfd                                ; Restore original EFLAGS
    and eax, 0x00200000                  ; Isolate ID bit (non-zero if supported)
    ret                                  ; Return value is already in EAX!
