; Константы Multiboot
MODULEALIGN equ  1<<0                   ; выравнивать модули по страницам
MEMINFO     equ  1<<1                   ; предоставлять карту памяти
FLAGS       equ  MODULEALIGN | MEMINFO  ; флаги multiboot
MAGIC       equ  0x1BADB002             ; магическое число
CHECKSUM    equ -(MAGIC + FLAGS)        ; контрольная сумма

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .text
global _start
extern kmain

_start:
    ; Установка стека (стек растет вниз)
    mov esp, stack_top
    
    ; Передача управления в Си
    push eax            ; заголовок multiboot magic
    push ebx            ; адрес структуры multiboot info
    call kmain

    ; Если ядро выйдет из kmain, останавливаем процессор
.hang:
    cli
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 16384 ; 16 КБ для стека
stack_top: