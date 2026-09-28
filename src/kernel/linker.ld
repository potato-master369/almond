ENTRY(_start)

KERNEL_PHYS   = 0x00100000;
KERNEL_VIRT   = 0xC8000000;
KMAIN_OFFSET  = 0x60000;

SECTIONS
{
    . = KERNEL_PHYS;

    .stub :
    {
        *stub.o(.text)
    }

    _lma = KERNEL_PHYS + KMAIN_OFFSET;
    . = KERNEL_VIRT + KMAIN_OFFSET;

    .text : AT(ADDR(.text) - KERNEL_VIRT + KERNEL_PHYS)
    {
        *(.text.kmain)
        *(.text .text.*)
    }
    .rodata : AT(ADDR(.rodata) - KERNEL_VIRT + KERNEL_PHYS)
    {
        *(.rodata .rodata.*)
    }
    .data : AT(ADDR(.data) - KERNEL_VIRT + KERNEL_PHYS)
    {
        *(.data .data.*)
    }
    .bss : AT(ADDR(.bss) - KERNEL_VIRT + KERNEL_PHYS)
    {
        _bss_start = .;
        *(COMMON)
        *(.bss .bss.*)
        _bss_end = .;
    }
    _lma = LOADADDR(.bss) + SIZEOF(.bss);
    _lma = ALIGN(_lma, 512) - 6;

    .trailer _lma : AT(_lma)
    {
        BYTE(0x61)
        BYTE(0x6c)
        BYTE(0x6d)
        BYTE(0x6f)
        BYTE(0x6e)
        BYTE(0x64)
    }

    _kernel_phys_end = _lma + SIZEOF(.trailer);
    _kernel_virt_end = _kernel_phys_end - KERNEL_PHYS + KERNEL_VIRT;

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note.*)
        *(.note.GNU-stack)
    }
}
