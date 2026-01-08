ENTRY(_start)
SECTIONS {
    . = 1M; /* Ядро грузится по адресу 1МБ */
    .text : { *(.multiboot) *(.text) }
    .data : { *(.data) }
    .bss  : { *(.bss) }
}