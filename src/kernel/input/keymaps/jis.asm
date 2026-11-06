; JIS Keymap
; ---------------------------
; Japanese (JIS 106/109) .akm source, romaji input
;
; Build with:
;   nasm -f bin jis.asm -o jis.akm
;
; Each data record is:
;   key   = (modifiers << 8) | HID keycode
;   value = ASCII byte
;
; Modifier bits (see keymap.h):
;   bit 0: capslock
;   bit 1: shift
;   bit 2: alt

org 0x0000
struc almond_keymap_t
    .magic: resb 4
    .len:   resd 1
endstruc

struc keymap_data_t
    .key:   resw 1
    .value: resb 1
endstruc

%define MOD_CAPS  0x01
%define MOD_SHIFT 0x02

; Emit the four case/shift combinations of a letter.
%macro letter 3 ; hid, unshifted, shifted
    dw 0x0000 | %1
    db %2
    dw (MOD_CAPS << 8) | %1
    db %3
    dw (MOD_SHIFT << 8) | %1
    db %3
    dw ((MOD_CAPS | MOD_SHIFT) << 8) | %1
    db %2
%endmacro

; Emit the four case/shift combinations of a non-letter key.
%macro symbol 3 ; hid, unshifted, shifted
    dw 0x0000 | %1
    db %2
    dw (MOD_CAPS << 8) | %1
    db %2
    dw (MOD_SHIFT << 8) | %1
    db %3
    dw ((MOD_CAPS | MOD_SHIFT) << 8) | %1
    db %3
%endmacro

section .data

_start:
  ; header
  db "akmp"
  dd (_end - _array_start) / 3

_array_start:
  ; Letters (romaji)
  letter 0x04, 'a', 'A'
  letter 0x05, 'b', 'B'
  letter 0x06, 'c', 'C'
  letter 0x07, 'd', 'D'
  letter 0x08, 'e', 'E'
  letter 0x09, 'f', 'F'
  letter 0x0A, 'g', 'G'
  letter 0x0B, 'h', 'H'
  letter 0x0C, 'i', 'I'
  letter 0x0D, 'j', 'J'
  letter 0x0E, 'k', 'K'
  letter 0x0F, 'l', 'L'
  letter 0x10, 'm', 'M'
  letter 0x11, 'n', 'N'
  letter 0x12, 'o', 'O'
  letter 0x13, 'p', 'P'
  letter 0x14, 'q', 'Q'
  letter 0x15, 'r', 'R'
  letter 0x16, 's', 'S'
  letter 0x17, 't', 'T'
  letter 0x18, 'u', 'U'
  letter 0x19, 'v', 'V'
  letter 0x1A, 'w', 'W'
  letter 0x1B, 'x', 'X'
  letter 0x1C, 'y', 'Y'
  letter 0x1D, 'z', 'Z'

  ; Number row (JIS 106: shift+0 is tilde)
  symbol 0x1E, '1', '!'
  symbol 0x1F, '2', '"'
  symbol 0x20, '3', '#'
  symbol 0x21, '4', '$'
  symbol 0x22, '5', '%'
  symbol 0x23, '6', '&'
  symbol 0x24, '7', 0x27 ; Apostrophe
  symbol 0x25, '8', '('
  symbol 0x26, '9', ')'
  symbol 0x27, '0', '~'

  ; Whitespace and control keys
  symbol 0x28, 0x0A, 0x0A ; Enter
  symbol 0x29, 0x1B, 0x1B ; Escape
  symbol 0x2A, 0x08, 0x08 ; Backspace
  symbol 0x2B, 0x09, 0x09 ; Tab
  symbol 0x2C, ' ', ' '   ; Space

  ; Symbol keys
  symbol 0x2D, '-', '='
  symbol 0x2E, '^', '~'
  symbol 0x2F, '@', '`'
  symbol 0x30, '[', '{'
  symbol 0x32, ']', '}'
  symbol 0x33, ';', '+'
  symbol 0x34, ':', '*'
  symbol 0x36, ',', '<'
  symbol 0x37, '.', '>'
  symbol 0x38, '/', '?'
  symbol 0x64, 0x5C, '|'  ; Non-US backslash (fallback)
  symbol 0x87, 0x5C, '_'  ; International1 (Ro)
  symbol 0x89, 0x5C, '|'  ; International3 (Yen)

_end:
