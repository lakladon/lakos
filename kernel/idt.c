/*
 * Lakos OS
 * Copyright (c) 2026 lakladon
 * Created: January 8, 2026
 */

#include <stdint.h>
#include <io.h>

// Структура IDT записи
struct idt_entry {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct idt_entry idt[256];
struct idt_ptr idtp;

// Эти функции мы объявим в ассемблере
extern void idt_load(uint32_t ptr);
extern void irq1(); // Обработчик клавиатуры
extern void irq12(); // Обработчик мыши

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void idt_init() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;

    // Очистка IDT
    for(int i = 0; i < 256; i++) idt_set_gate(i, 0, 0, 0);

    idt_load((uint32_t)&idtp);
}

// РЕАЛИЗАЦИЯ irq_install (Ремаппинг PIC и регистрация клавиатуры)
void irq_install() {
    // Инициализация PIC (ICW1)
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    // Ремаппинг базовых векторов (ICW2)
    outb(0x21, 0x20); // IRQ 0-7 -> 32-39
    outb(0xA1, 0x28); // IRQ 8-15 -> 40-47
    // Настройка связей (ICW3)
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    // Режим 8086 (ICW4)
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    // Маски прерываний (включаем клавиатуру IRQ1 и мышь IRQ12)
    outb(0x21, 0xFD); // 0xFD = 11111101 (бит 1 включен)
    outb(0xA1, 0xEF); // 0xEF = 11101111 (бит 4 включен для IRQ12)

    // Регистрируем обработчик клавиатуры на вектор 33
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);
    // Регистрируем обработчик мыши на вектор 44
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
}