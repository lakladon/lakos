; Исправленный фрагмент kernel/interrupts.asm
[BITS 32]
extern isr_handler    ; Исправлено с heandler на handler

isr_common_stub:
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    call isr_handler  ; Исправлено с heandler на handler
    
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    iret
