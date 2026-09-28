MEMORY
{
    /* Loaded at physical address 0x7E00 for a strict length of 31.5 KiB */
    ROM (rx) : ORIGIN = 0x7E00, LENGTH = 32256
}

SECTIONS
{
    .text : {
        /* Ensure the entry point stub comes first */
        *(.text._start)
        *(.text*)
        *(.rodata*)
        *(.data*)
    } > ROM

    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    } > ROM
    
    /* Place "almond" at the exact end of the 31.5 KiB region */
    .magic (ORIGIN(ROM) + LENGTH(ROM) - 6) : {
        BYTE(0x61) /* 'a' */
        BYTE(0x6c) /* 'l' */
        BYTE(0x6d) /* 'm' */
        BYTE(0x6f) /* 'o' */
        BYTE(0x6e) /* 'n' */
        BYTE(0x64) /* 'd' */
    } > ROM = 0x00
}
