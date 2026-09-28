[bits 32]
global _start
extern kmain

%define KERNEL_STACK_TOP 0xC9F00000

section .text
_start:
    mov esi, [esp + 4]
    mov esp, KERNEL_STACK_TOP
   
    ; reset stupid paging CR4.PAE
    mov eax, cr4
    and eax, ~(1 << 5)    ; Clear bit 5
    mov cr4, eax

    mov eax, page_directory
    sub eax, 0xc7F00000
    mov cr3, eax

    ; enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; jump to kernel
    mov esp, KERNEL_STACK_TOP
    push esi
    call kmain

hcf:
    jmp hcf

section .data

align 4096
page_directory:
    %assign i 0
    %rep 8
        dd (identity_pt + (i * 4096) + 0x03 - 0xC7F00000)
        %assign i i+1
    %endrep
    times 792 dd 0x0
    %assign i 0
    %rep 8
        dd (higher_half_pt + (i * 4096) + 0x03 - 0xC7F00000)
        %assign i i+1
    %endrep
    times 216 dd 0x0

align 4096
identity_pt:
    %assign i 0
    %rep 8192
        dd (i * 4096) + 0x03
        %assign i i+1
    %endrep

align 4096
higher_half_pt:
    %assign i 0
    %rep 8192
        dd (0x00100000 + (i * 4096)) + 0x03
        %assign i i+1
    %endrep
