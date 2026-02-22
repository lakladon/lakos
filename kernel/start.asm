[BITS 64]
section .text
global _start
extern kmain

_start:
    mov rsp, 0x200000
    mov rbp, 0x200000
    push rbx
    push rax
    call kmain
    hlt
