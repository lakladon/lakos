[BITS 32]
[GLOBAL idt_load]
[EXTERN isr_heandler]
%macro ISR_NOERR 1
[GLOBAL isr%1]
isr%1:
    push byte 0
    push byte %1 
    jmp isr_common_stub
%endmacro

%macro ISR_ERR 1
[GLOBAL isr%1]
isr%1:
    push byte %1 
    jmp isr_common_stub
%endmacro

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 8
ISR_NOERR 13
ISR_NOERR 14

isr_common_stub:
    pusha
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call isr_heandler

    pop eax
    mov ds, ax
    mov es, ax  
    mov fs, ax
    mov gs, ax

    popa 
    add esp, 8
    iret
    