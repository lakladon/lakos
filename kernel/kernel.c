#include "include/multiboot.h"
#include "include/gdt.h"
#include "include/idt.h"
#include <stdint.h>
#include <stddef.h>

/* --- Константы VGA --- */
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;

/* --- Глобальное состояние терминала --- */
size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

/* --- Базовые функции строк --- */
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = 0x07;
    terminal_buffer = VGA_MEMORY;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer[y * VGA_WIDTH + x] = (uint16_t)' ' | (uint16_t)terminal_color << 8;
        }
    }
}

void terminal_scroll() {
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer[y * VGA_WIDTH + x] = terminal_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (uint16_t)' ' | (uint16_t)terminal_color << 8;
    }
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll();
            terminal_row = VGA_HEIGHT - 1;
        }
        return;
    }

    const size_t index = terminal_row * VGA_WIDTH + terminal_column;
    terminal_buffer[index] = (uint16_t)c | (uint16_t)terminal_color << 8;
    
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll();
            terminal_row = VGA_HEIGHT - 1;
        }
    }
}

void terminal_writestring(const char* data) {
    for (size_t i = 0; data[i] != '\0'; i++) terminal_putchar(data[i]);
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

// Прототипы функций прерываний (должны быть в idt.c/irq.c)
extern void idt_init();
extern void isrs_install();
extern void irq_install();
extern void shell_main();

void* initrd_location = NULL;

void kmain(multiboot_info_t* mb_info, uint32_t magic) {
    // 1. Сначала инициализируем видео, чтобы видеть ошибки
    terminal_initialize();

    // 2. Базовая инициализация железа
    init_gdt();
    idt_init();       
    isrs_install();   
    irq_install();    

    terminal_setcolor(0x0B);
    terminal_writestring("Lakos OS Kernel v0.3 Booting...\n");
    terminal_setcolor(0x07);

    // 3. Проверка Multiboot Magic
    if (magic != 0x2BADB002) {
        terminal_writestring("Error: Invalid Multiboot magic number.\n");
        return;
    }

    // 4. Поиск модулей (InitRD)
    if (mb_info->flags & (1 << 3)) { 
        if (mb_info->mods_count > 0) {
            multiboot_module_t* mod = (multiboot_module_t*)mb_info->mods_addr;
            initrd_location = (void*)mod->mod_start;
            terminal_writestring("[ OK ] InitRD found at memory address.\n");
        }
    } else {
        terminal_writestring("[ WARN ] No InitRD modules detected.\n");
    }

    terminal_writestring("GDT/IDT loaded. Memory mapped. Entering Shell...\n\nLakos> ");

    // 5. Включаем прерывания перед запуском шелла
    asm volatile("sti");

    // 6. Запуск основной программы
    shell_main();

    // 7. Если вышли из шелла — вечный сон
    while(1) {
        asm volatile("hlt");
    }
}