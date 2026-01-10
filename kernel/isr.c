#include <stdint.h>
#include <io.h>

extern void terminal_putchar(char c);

// Таблица скан-кодов
unsigned char kbdus[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0
};

void isr_handler(uint32_t int_no) {
    // 1. Обрабатываем клавиатуру
    if (int_no == 33) {
        uint8_t scancode = inb(0x60); 
        if (!(scancode & 0x80)) {
            char c = kbdus[scancode];
            if (c != 0) terminal_putchar(c);
        }
    }

    // 2. Отправляем сигнал завершения (EOI) в PIC
    if (int_no >= 40) {
        outb(0xA0, 0x20); 
    }
    outb(0x20, 0x20); 
} // <- УБЕДИСЬ, ЧТО ЭТА СКОБКА ЕСТЬ
