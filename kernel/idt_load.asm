[bits 64]
global idt_load
extern idtp ; Ссылка на структуру из idt.c

idt_load:
    lidt [rel idtp] ; Загружаем адрес и лимит IDT (RIP-relative в 64-bit)
    ret
