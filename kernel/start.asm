section .text
global _start
extern kmain

_start:
    mov esp, 0x200000
    mov ebp, 0x200000
    push ebx
    push eax
    call kmain
    hlt