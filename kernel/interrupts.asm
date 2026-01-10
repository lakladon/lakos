[bits 32]
extern isr_handler
global irq1

irq1:
    push byte 0      ; Фиктивный код ошибки
    push byte 33     ; Номер прерывания (IRQ1 + 32)
    pusha            ; Сохраняем все регистры (EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)

    mov ax, ds       ; Сохраняем сегмент данных
    push eax

    mov ax, 0x10     ; Загружаем сегмент данных ядра
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call isr_handler ; ВЫЗОВ ТВОЕГО СИ-КОДА

    pop eax          ; Восстанавливаем оригинальный сегмент данных
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa             ; Восстанавливаем регистры
    add esp, 8       ; Очищаем стек (от номера и кода ошибки)
    iret             ; ВОЗВРАТ ИЗ ПРЕРЫВАНИЯ - это оживит процессор