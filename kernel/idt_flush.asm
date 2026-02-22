[bits 64]
global idt_flush

idt_flush:
    mov rax, [rsp + 8]  ; Получаем указатель на idt_ptr из стека
    lidt [rax]          ; Загружаем IDT
    ret
