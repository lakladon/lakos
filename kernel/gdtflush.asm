[BITS 32]
global gdt_flush     ; Объявляем функцию глобальной для линковщика

gdt_flush:
    mov eax, [esp + 4] ; Берем указатель на gdt_ptr из стека
    lgdt [eax]         ; Загружаем GDT

    ; Перезагружаем сегменты данных
    mov ax, 0x10       ; 0x10 — смещение сегмента данных в GDT
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Дальний прыжок для перезагрузки сегмента кода (CS)
    jmp 0x08:.flush    ; 0x08 — смещение сегмента кода

.flush:
    ret