[bits 64]
extern isr_handler
global irq1
global irq12

irq1:
    push byte 0      ; Фиктивный код ошибки
    push byte 33     ; Номер прерывания (IRQ1 + 32)
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov ax, ds       ; Сохраняем сегмент данных
    push rax

    mov ax, 0x10     ; Загружаем сегмент данных ядра
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call isr_handler ; ВЫЗОВ СИ-КОДА

    pop rax          ; Восстанавливаем оригинальный сегмент данных
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    add rsp, 16      ; Очищаем стек (от номера и кода ошибки)
    iretq            ; ВОЗВРАТ ИЗ ПРЕРЫВАНИЯ в 64-bit режиме

irq12:
    push byte 0      ; Фиктивный код ошибки
    push byte 44     ; Номер прерывания (IRQ12 + 32)
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov ax, ds       ; Сохраняем сегмент данных
    push rax

    mov ax, 0x10     ; Загружаем сегмент данных ядра
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call isr_handler ; ВЫЗОВ СИ-КОДА

    pop rax          ; Восстанавливаем оригинальный сегмент данных
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    add rsp, 16      ; Очищаем стек (от номера и кода ошибки)
    iretq            ; ВОЗВРАТ ИЗ ПРЕРЫВАНИЯ в 64-bit режиме
