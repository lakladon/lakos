; Lakos OS
; Copyright (c) 2026 lakladon
; Created: January 8, 2026

[global idt_flush]

idt_flush:
    mov eax, [esp + 4]  ; Получаем указатель на idt_ptr из стека
    lidt [eax]          ; Загружаем IDT
    ret