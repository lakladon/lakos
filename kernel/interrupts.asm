[extern isr_handler]
[global irq1]
[global irq12]

irq1:
    cli             ; Выключаем прерывания
    push byte 0     ; Ошибка (заглушка)
    push byte 33    ; Номер прерывания (32 + 1)
    jmp irq_common_stub

irq12:
    cli
    push byte 0
    push byte 44    ; 32 + 12
    jmp irq_common_stub

irq_common_stub:
    pushad          ; Сохраняем EAX, ECX, EDX...
    
    mov ax, ds      ; Сохраняем сегмент данных
    push eax

    mov ax, 0x10    ; Загружаем сегмент данных ядра
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call isr_handler ; Вызов Си-функции из kernel/isr.c

    pop eax         ; Восстанавливаем сегменты
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popad           ; Восстанавливаем регистры
    add esp, 8      ; Очищаем стек от номера прерывания и ошибки
    sti
    iret            ; Возврат из прерывания!
