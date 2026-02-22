[BITS 64]
global gdt_flush     ; Объявляем функцию глобальной для линковщика

gdt_flush:
    mov rax, [rsp + 8] ; Берем указатель на gdt_ptr из стека
    lgdt [rax]         ; Загружаем GDT

    ; Перезагружаем сегменты данных
    mov ax, 0x10       ; 0x10 — смещение сегмента данных в GDT
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Дальний прыжок для перезагрузки сегмента кода (CS) в 64-bit режиме
    push 0x08                ; Селектор сегмента кода
    lea rax, [rel .flush]    ; Адрес метки .flush
    push rax                 ; Помещаем адрес в стек
    retfq                    ; Far return - прыжок на новый CS:RIP

.flush:
    ret
