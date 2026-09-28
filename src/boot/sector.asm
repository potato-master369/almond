; NASM syntax
; -------------------------------------------
; Boot sector code for almond. This is really
; hacky and I honestly wonder if this even
; is good enough.

; MIT license; Copyright (C) potato-master369 2026-

[bits 16]
[org 0x7c00]

global _start

_start:
    cli
    cld

    ; save the drive BIOS gave us
    mov [boot_drive], dl

    ; reset registers
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7000 ; stack pointer

    ; clear screen
    mov ax, 0x0003
    int 0x10

    ; load bootloader stage 2 into RAM
    mov ax, 0x7e0 ; right after boot sector
    mov es, ax
    xor bx, bx

    ; params for int 13h
    mov ah, 0x42
    mov si, dap

    int 0x13
    jc disk_error
    jmp 0x7e0:0x0000

disk_error:
    ; something went wrong!
    ; fallback: CHS read
    mov ah, 0x02
    mov al, 63
    mov ch, 0
    mov dh, 0
    mov cl, 2
    mov bx, 0x7e0
    mov es, bx
    xor bx, bx
    int 0x13
    jc disk_error2
    jmp 0x730:0x0000
disk_error2:
    mov ah, 0x1E
    mov si, message1
    call print_string
    cli
    ; halt and catch fire
hcf:
    hlt
    jmp hcf

print_string:
    push ax
    push bx
    push dx
    push si
    push es

    mov cl, ah
    mov ax, 0xB800
    mov es, ax

    movzx bx, byte [cursor_y]
    imul bx, bx, 80
    movzx dx, byte [cursor_x]
    add bx, dx
    shl bx, 1
    mov dl, ah

.loop:
    lodsb
    cmp al, 0
    je .done

    cmp al, 0x0A
    je .newline

    mov ah, dl
    mov [es:bx], al
    mov [es:bx+1], cl
    add bx, 2
    
    inc byte [cursor_x]
    cmp byte [cursor_x], 80
    jl .loop

.newline:
    mov byte [cursor_x], 0
    inc byte [cursor_y]
    
    movzx bx, byte [cursor_y]
    imul bx, bx, 80
    shl bx, 1
    jmp .loop

.done:
    pop es
    pop si
    pop dx
    pop bx
    pop ax
    ret

cursor_x: db 0
cursor_y: db 0
boot_drive: db 0

message1: db `ABS: E0001 could not load stage 2 bootloader.\n    Possible fixes: check disk is intact, and that sectors 2-63 contain stage\n    II.\n`
dap:
    db 0x10       ; Size of packet (16 bytes)
    db 0          ; Reserved
    dw 63         ; Number of sectors to read (31.5 KiB = 63 sectors)
    dw 0x0000     ; Destination Offset
    dw 0x07E0     ; Destination Segment (0x07E0:0x0000 = 0x7E00)
    dq 1          ; Starting LBA sector (Sector 1, right after boot sector)
; appease the bios
times 510-($-$$) db 0
dw 0xAA55 
