#include <stdint.h>
#include "include/idt.h"

// Объявляем таблицу прерываний
idt_entry_t idt_entries[256];

// Переименовано в idtp для соответствия ассемблеру
idt_ptr_t idtp; 

extern void idt_flush(uint32_t);
extern void idt_load(); // Если используешь idt_load.asm

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_low = base & 0xFFFF;
    idt_entries[num].base_high = (base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags   = flags | 0x60; // 0x60 обычно добавляется для пользовательских прерываний
}

void init_idt() {
    idtp.limit = (sizeof(idt_entry_t) * 256) - 1;
    idtp.base  = (uint32_t)&idt_entries;

    // Очистка таблицы
    for(int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    // Загрузка
    idt_flush((uint32_t)&idtp);
    // Если твой Makefile использует idt_load.asm, вызови и его:
    // idt_load(); 
}
