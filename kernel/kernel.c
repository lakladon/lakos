#include "include/multiboot.h"
#include <stdint.h>
#include <stddef.h>

// Multiboot header
const multiboot_header_t __attribute__((section(".multiboot"))) header = {
    .magic = MULTIBOOT_HEADER_MAGIC,
    .flags = 0,
    .checksum = -(MULTIBOOT_HEADER_MAGIC + 0)
};

// --- РЕАЛИЗАЦИЯ ФУНКЦИЙ ЭКРАНА ---
uint16_t* vga_buffer = (uint16_t*)0xB8000;
int term_col = 0;
int term_row = 0;

void terminal_initialize() {
    term_col = 0;
    term_row = 0;
    for (int y = 0; y < 25; y++) {
        for (int x = 0; x < 80; x++) {
            vga_buffer[y * 80 + x] = (uint16_t)' ' | (uint16_t)0x07 << 8;
        }
    }
}

void terminal_putchar(char c) {
    if (c == '\n') {
        term_col = 0;
        term_row++;
    } else if (c == '\b') {
        if (term_col > 0) {
            term_col--;
            vga_buffer[term_row * 80 + term_col] = (uint16_t)' ' | (uint16_t)0x07 << 8;
        }
    } else {
        vga_buffer[term_row * 80 + term_col] = (uint16_t)c | (uint16_t)0x07 << 8;
        term_col++;
    }

    if (term_col >= 80) {
        term_col = 0;
        term_row++;
    }

    if (term_row >= 25) {
        // Scroll up
        for (int y = 1; y < 25; y++) {
            for (int x = 0; x < 80; x++) {
                vga_buffer[(y-1)*80 + x] = vga_buffer[y*80 + x];
            }
        }
        // Clear last line
        for (int x = 0; x < 80; x++) {
            vga_buffer[24*80 + x] = (uint16_t)' ' | (uint16_t)0x07 << 8;
        }
        term_row = 24;
    }
}

void terminal_writestring(const char* s) {
    for (int i = 0; s[i] != '\0'; i++) terminal_putchar(s[i]);
}

// Прототипы
void init_gdt();
void idt_init();
void irq_install();
extern void mouse_install();
extern void shell_main();

void kmain(multiboot_info_t* mb_info, uint32_t magic) {
    terminal_initialize();
    init_gdt();
    idt_init();
    irq_install();
    mouse_install();

    terminal_writestring("Lakos OS v0.3 Booted!\n");

    __asm__ volatile("sti");
    shell_main();

    while(1) { __asm__ volatile("hlt"); }
}