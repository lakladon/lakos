MODULEALIGN equ  1<<0                   
MEMINFO     equ  1<<1                   
FLAGS       equ  MODULEALIGN | MEMINFO  
MAGIC       equ  0x1BADB002             
CHECKSUM    equ -(MAGIC + FLAGS)        
section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
section .text
global _start
extern kmain
_start:
    mov esp, stack_top
    push eax           
    push ebx            
    call kmain
.hang:
    cli
    hlt
    jmp .hang
section .bss
align 16
stack_bottom:
    resb 16384 
stack_top: