; Lakos OS
; Copyright (c) 2026 lakladon
; Created: January 8, 2026

[bits 32]
global idt_load
extern idtp ; Ссылка на структуру из idt.c

idt_load:
    lidt [idtp] ; Загружаем адрес и лимит IDT
    ret