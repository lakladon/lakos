#include <stdint.h>
#include <io.h>

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

extern void idt_load(uint32_t ptr); 
extern void irq1(); 

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void default_handler() {
    outb(0x20, 0x20); 
    outb(0xA0, 0x20);
}

void pic_remap() {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    // Маскируем все прерывания
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

void idt_init() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;

    for(int i = 0; i < 256; i++) {
        idt_set_gate(i, (uint32_t)default_handler, 0x08, 0x8E);
    }

    idt_load((uint32_t)&idtp);
}

// ТОЛЬКО ОДНА ФУНКЦИЯ irq_install
void irq_install() {
    pic_remap();
    
    // Устанавливаем обработчик клавиатуры
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);

    // Разрешаем только клавиатуру (IRQ1)
    outb(0x21, 0xFD); 
    outb(0xA1, 0xFF);
}