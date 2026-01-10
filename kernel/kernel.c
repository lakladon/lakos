#include "include/multiboot.h"
#include <stdint.h>
#include <stddef.h>

// --- Реализация функций терминала (были потеряны) ---
void terminal_initialize() {
    uint16_t* vga_buffer = (uint16_t*)0xB8000;
    for (int i = 0; i < 80 * 25; i++) vga_buffer[i] = (uint16_t)' ' | (uint16_t)0x07 << 8;
}

void terminal_putchar(char c) {
    // Упрощенная версия для теста
    static uint16_t* vga = (uint16_t*)0xB8000;
    static int cursor = 0;
    if (c == '\n') cursor = (cursor / 80 + 1) * 80;
    else vga[cursor++] = (uint16_t)c | (uint16_t)0x07 << 8;
}

void terminal_writestring(const char* s) {
    for (int i = 0; s[i] != '\0'; i++) terminal_putchar(s[i]);
}

// Прототипы
void init_gdt();
void idt_init();
void irq_install();
extern void shell_main();

void kmain(multiboot_info_t* mb_info, uint32_t magic) {
    terminal_initialize();
    init_gdt();
    idt_init();
    irq_install();

    terminal_writestring("Lakos OS v0.3 Booted!\n");

    __asm__ volatile("sti");
    shell_main();

    while(1) { __asm__ volatile("hlt"); }
}