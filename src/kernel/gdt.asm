[bits 32]

global gdt_install
global tss_load
global gdt_set_tss

gdt_install:
    lgdt [gdt_descriptor]
    jmp 0x08:.reload_cs

.reload_cs:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret

; void tss_load(uint16_t selector)
tss_load:
    mov ax, [esp + 4]
    ltr ax
    ret

; void gdt_set_tss(uint32_t base, uint32_t limit)
gdt_set_tss:
    push ebp
    mov ebp, esp
    push eax
    push ebx
    push edi

    ; GDT entry 3 = gdt_start + 24
    mov edi, gdt_start + 24

    mov eax, [ebp + 8]     ; TSS base
    mov ebx, [ebp + 12]    ; TSS limit

    mov [edi], bx          ; Limit low
    mov [edi + 2], ax      ; Base low

    shr eax, 16
    mov [edi + 4], al      ; Base middle

    mov byte [edi + 5], 0x89 ; Available 32-bit TSS

    mov eax, ebx
    shr eax, 16
    and al, 0x0F
    mov [edi + 6], al      ; Limit high

    mov eax, [ebp + 8]
    shr eax, 24
    mov [edi + 7], al      ; Base high

    pop edi
    pop ebx
    pop eax
    pop ebp
    ret

section .data
align 8

gdt_start:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
    dq 0x0000000000000000 ; TSS entry, populated at runtime
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start
