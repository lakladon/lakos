#include "include/multiboot.h"
#include <stdint.h>
#include <stddef.h>

// Multiboot header
const multiboot_header_t __attribute__((section(".multiboot"))) header = {
    .magic = MULTIBOOT_HEADER_MAGIC,
    .flags = 0,
    .checksum = -(MULTIBOOT_HEADER_MAGIC + 0)
};

void _start() {
    kmain(0, 0);
}

// Graphical terminal
extern void vga_clear_screen(uint8_t color);
extern void vga_draw_char(int x, int y, char c, uint8_t color);

int term_col = 0;
int term_row = 0;

void terminal_initialize() {
    term_col = 0;
    term_row = 0;
    vga_clear_screen(0); // black background
}

void terminal_putchar(char c) {
    if (c == '\n') {
        term_col = 0;
        term_row++;
    } else if (c == '\b') {
        if (term_col > 0) {
            term_col--;
            vga_draw_char(term_col * 8, term_row * 8, ' ', 0);
        }
    } else {
        vga_draw_char(term_col * 8, term_row * 8, c, 15); // white
        term_col++;
    }

    if (term_col >= 40) { // 320 / 8 = 40 chars per line
        term_col = 0;
        term_row++;
    }

    if (term_row >= 25) { // scroll, but for simplicity, reset
        term_row = 0;
        vga_clear_screen(0);
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
extern void vga_set_mode_13h();
extern void shell_main();

void kmain(multiboot_info_t* mb_info, uint32_t magic) {
    vga_set_mode_13h();
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