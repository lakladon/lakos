#include <stdint.h>
#include "include/idt.h"

// Ошибка была здесь: пропущен # и массив не был объявлен
// Объявляем саму таблицу прерываний на 256 записей
idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

// Внешняя функция на ассемблере для загрузки IDT
extern void idt_flush(uint32_t);

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_low = base & 0xFFFF;
    idt_entries[num].base_high = (base >> 16) & 0xFFFF;

    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags   = flags;
}

void init_idt() {
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    // Здесь ты обычно заполняешь таблицу (например, через idt_set_gate)
    
    idt_flush((uint32_t)&idt_ptr);
}
