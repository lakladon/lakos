/*
 * Lakos OS
 * Copyright (c) 2026 lakladon
 * Created: January 8, 2026
 */

#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// Структура записи IDT
struct idt_entry_struct {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed)); // ИСПРАВЛЕНО: должно быть два подчеркивания в начале

typedef struct idt_entry_struct idt_entry_t;

// Структура указателя IDT (для команды lidt)
struct idt_ptr_struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)); // ИСПРАВЛЕНО: должно быть два подчеркивания в начале

typedef struct idt_ptr_struct idt_ptr_t;

#endif
