[BITS 32]
global idt_load
extern idtp ; это указатель на структуру IDT, проверь имя в idt.c

idt_load:
    lidt [idtp]
    ret
