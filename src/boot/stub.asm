; stub.asm
; ----------------------
; MIT license; Copyright (C) potato-master369 2026-
; stub to load bootloader stage 2

[bits 16]

global _start
global mmap_buf
global mmap_count
global bl_vbe_framebuffer
global bl_vbe_pitch
global bl_vbe_width
global bl_vbe_height
global bl_vbe_bpp
global bl_vbe_red_mask
global bl_vbe_red_position
global bl_vbe_green_mask
global bl_vbe_green_position
global bl_vbe_blue_mask
global bl_vbe_blue_position
extern bl_main
mmap_buf equ 0x90000
mmap_count equ 0x90600
mmap_count_offset equ 0x0600
_start:
    cli

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7000
    ; E820: detect memory
    mov ax, 0x9000
    mov es, ax
    xor di, di
detect_memory:
    xor ebx, ebx
    xor bp, bp
    mov edx, 0x534D4150
.loop:
    mov ax, 0x9000
    mov es, ax
    push di
    push bp
    mov eax, 0xE820
    mov ecx, 24
    mov edx, 0x534D4150 ; SMAP
    
    ; for shitty buggy BIOS
    mov dword[es:di + 20], 1
    int 0x15
    pop bp
    pop di
    jc .check_done

    cmp eax, edx
    jne .failed

    jcxz .skip_entry ; validate size returned by BIOS
    cmp ecx, 20
    jge .process_entry
    jmp .failed
.process_entry:
    cmp ecx, 24
    jl .valid_entry
    test dword [es:di + 20], 1 ; ignore bit
    jz .skip_entry
.valid_entry:
    inc bp
    add di, 24
.skip_entry:
    test ebx, ebx
    jne .loop
    jmp .done
.check_done:
    test bp, bp
    jne .done
.failed:
    hlt
    jmp .failed ; hcf
.done:
    clc
    mov [es:mmap_count_offset], bp
    xor ax, ax
    mov ds, ax
    mov es, ax
    sti
    call vbe_init
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    jmp 0x08:init_pm

[bits 32]
init_pm:
    ; we are in 32-bit mode now
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ; A20 line
    in al, 0x92
    or al, 2
    and al, 0xFE
    out 0x92, al
    ; update stack pointer
    mov ebp, 0x02000000
    mov esp, ebp
    ; ----------------------------------
    ; | call the bootloader            |
    ; ----------------------------------
    cli
    call bl_main
hcf:
    hlt
    jmp hcf

[bits 16]
vbe_init:
    mov dword [vbe_controller], 'VBE2'
    xor ax, ax
    mov es, ax
    mov di, vbe_controller
    mov ax, 0x4F00
    int 0x10
    cmp ax, 0x004F
    jne .done
    mov ax, [vbe_controller + 0x12]
    mov [vbe_memory_blocks], ax
    mov byte [vbe_edid_available], 0

    mov ax, 0x4F15
    xor bx, bx
    xor cx, cx
    int 0x10
    cmp ax, 0x004F
    jne .mode_list
    cmp bh, 1
    jb .mode_list

    xor ax, ax
    mov es, ax
    mov di, vbe_edid
    mov ax, 0x4F15
    mov bl, 1
    xor cx, cx
    int 0x10
    cmp ax, 0x004F
    jne .mode_list
    cmp byte [vbe_edid], 0
    jne .mode_list
    cmp byte [vbe_edid + 1], 0xFF
    jne .mode_list

    mov word [vbe_edid_width], 0
    mov word [vbe_edid_height], 0
    mov si, vbe_edid + 54
    mov cx, 4
.edid_timing:
    cmp word [si], 0
    je .next_timing
    xor bx, bx
    mov bl, [si + 2]
    xor ax, ax
    mov al, [si + 4]
    and al, 0xF0
    shl ax, 4
    or ax, bx
    mov [vbe_timing_width], ax
    xor bx, bx
    mov bl, [si + 5]
    xor ax, ax
    mov al, [si + 7]
    and al, 0xF0
    shl ax, 4
    or ax, bx
    mov bx, [vbe_timing_width]
    cmp bx, [vbe_edid_width]
    ja .save_timing
    jb .next_timing
    cmp ax, [vbe_edid_height]
    jbe .next_timing
.save_timing:
    mov bx, [vbe_timing_width]
    mov [vbe_edid_width], bx
    mov [vbe_edid_height], ax
.next_timing:
    add si, 18
    loop .edid_timing
    cmp word [vbe_edid_width], 0
    je .mode_list
    mov byte [vbe_edid_available], 1

.mode_list:
    les si, [vbe_controller + 14]
    mov word [vbe_best_width], 0
.mode_loop:
    lodsw
    cmp ax, 0xFFFF
    je .set_mode
    mov [vbe_mode], ax
    push ds
    push es
    push si
    xor ax, ax
    mov es, ax
    mov di, vbe_mode_info
    mov ax, 0x4F01
    mov cx, [vbe_mode]
    int 0x10
    pop si
    pop es
    pop ds
    cmp ax, 0x004F
    jne .mode_loop
    mov ax, [vbe_mode_info]
    and ax, 0x0091
    cmp ax, 0x0091
    jne .mode_loop
    mov al, [vbe_mode_info + 0x19]
    cmp al, 15
    je .bpp_valid
    cmp al, 16
    je .bpp_valid
    cmp al, 24
    je .bpp_valid
    cmp al, 32
    jne .mode_loop
.bpp_valid:
    cmp byte [vbe_mode_info + 0x1B], 6
    jne .mode_loop
    mov ax, [vbe_mode_info + 0x12]
    mov bx, [vbe_mode_info + 0x14]
    movzx eax, word [vbe_mode_info + 0x10]
    movzx ecx, bx
    mul ecx
    test edx, edx
    jnz .mode_loop
    movzx ecx, word [vbe_memory_blocks]
    shl ecx, 16
    cmp eax, ecx
    ja .mode_loop
        mov edi, [vbe_mode_info + 0x28]
        add eax, edi
        adc edx, 0
        jnz .mode_loop
    mov ax, [vbe_mode_info + 0x12]
    mov bx, [vbe_mode_info + 0x14]
    cmp byte [vbe_edid_available], 0
    je .resolution_valid
    cmp ax, [vbe_edid_width]
    ja .mode_loop
    cmp bx, [vbe_edid_height]
    ja .mode_loop
.resolution_valid:
    cmp ax, [vbe_best_width]
    ja .save_mode
    jb .mode_loop
    cmp bx, [vbe_best_height]
    ja .save_mode
    jb .mode_loop
    mov al, [vbe_mode_info + 0x19]
    mov cl, [vbe_best_bpp]
    cmp al, cl
    jbe .mode_loop
.save_mode:
    mov [vbe_best_width], ax
    mov [vbe_best_height], bx
    mov ax, [vbe_mode]
    mov [vbe_best_mode], ax
    mov al, [vbe_mode_info + 0x19]
    mov [vbe_best_bpp], al
    jmp .mode_loop

.set_mode:
    cmp word [vbe_best_width], 0
    je .done
    xor ax, ax
    mov es, ax
    mov di, vbe_mode_info
    mov ax, 0x4F01
    mov cx, [vbe_best_mode]
    int 0x10
    cmp ax, 0x004F
    jne .done
    mov bx, [vbe_best_mode]
    or bx, 0x4000
    mov ax, 0x4F02
    int 0x10
    cmp ax, 0x004F
    jne .done
    mov eax, [vbe_mode_info + 0x28]
    mov [bl_vbe_framebuffer], eax
    mov ax, [vbe_mode_info + 0x10]
    mov [bl_vbe_pitch], ax
    mov ax, [vbe_mode_info + 0x12]
    mov [bl_vbe_width], ax
    mov ax, [vbe_mode_info + 0x14]
    mov [bl_vbe_height], ax
    mov al, [vbe_mode_info + 0x19]
    mov [bl_vbe_bpp], al
    mov al, [vbe_mode_info + 0x1F]
    mov [bl_vbe_red_mask], al
    mov al, [vbe_mode_info + 0x20]
    mov [bl_vbe_red_position], al
    mov al, [vbe_mode_info + 0x21]
    mov [bl_vbe_green_mask], al
    mov al, [vbe_mode_info + 0x22]
    mov [bl_vbe_green_position], al
    mov al, [vbe_mode_info + 0x23]
    mov [bl_vbe_blue_mask], al
    mov al, [vbe_mode_info + 0x24]
    mov [bl_vbe_blue_position], al
.done:
    ret


; GDT stuf
; ----------------------
align 4
gdt_start:
    ; null desc
    dd 0x0, 0x0

gdt_code:
    dw 0xffff
    dw 0x0
    db 0x0
    db 10011010b
    db 11001111b
    db 0x0

gdt_data:
    dw 0xffff
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; sizeof GDT
    dd gdt_start
bl_vbe_framebuffer: dd 0
bl_vbe_pitch: dw 0
bl_vbe_width: dw 0
bl_vbe_height: dw 0
bl_vbe_bpp: db 0
bl_vbe_red_mask: db 0
bl_vbe_red_position: db 0
bl_vbe_green_mask: db 0
bl_vbe_green_position: db 0
bl_vbe_blue_mask: db 0
bl_vbe_blue_position: db 0
vbe_mode: dw 0
vbe_best_mode: dw 0
vbe_best_width: dw 0
vbe_best_height: dw 0
vbe_best_bpp: db 0
vbe_edid_width: dw 0
vbe_edid_height: dw 0
vbe_timing_width: dw 0
vbe_edid_available: db 0
vbe_memory_blocks: dw 0
vbe_controller: resb 512
vbe_mode_info: resb 256
vbe_edid: resb 128
