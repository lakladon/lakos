[bits 32]
global idt_load
extern idtp ; Ссылка на структуру из idt.c

idt_load:
    lidt [idtp] ; Загружаем адрес и лимит IDT
    ret