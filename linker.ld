OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)

SECTIONS
{
    /* Ядро обычно загружается по адресу 1МБ */
    . = 1M;

    .text BLOCK(4K) : ALIGN(4K)
    {
        /* Обязательно первым! */
        KEEP(*(.multiboot))
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
}