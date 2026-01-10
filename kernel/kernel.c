#include "include/multiboot.h"
#include <stdint.h>
#include <stddef.h>

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void kmain(multiboot_info_t* mb_info, uint32_t magic);

// Multiboot header
const multiboot_header_t __attribute__((section(".multiboot"))) header = {
    .magic = MULTIBOOT_HEADER_MAGIC,
    .flags = 0,
    .checksum = -(MULTIBOOT_HEADER_MAGIC + 0)
};

void _start() {
    kmain(0, 0);
}

// Text mode terminal
#define VIDEO_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

uint16_t* video_memory = (uint16_t*)VIDEO_MEMORY;

int term_col = 0;
int term_row = 0;

void update_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void terminal_initialize() {
    term_col = 0;
    term_row = 0;
    // Clear screen
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        video_memory[i] = (uint16_t)' ' | (uint16_t)0x0F << 8; // white on black
    }
    // Set cursor position (optional, but for completeness)
    update_cursor(0, 0);
}

void terminal_putchar(char c) {
    if (c == '\n') {
        term_col = 0;
        term_row++;
    } else if (c == '\b') {
        if (term_col > 0) {
            term_col--;
            video_memory[term_row * VGA_WIDTH + term_col] = (uint16_t)' ' | (uint16_t)0x0F << 8;
        }
    } else {
        video_memory[term_row * VGA_WIDTH + term_col] = (uint16_t)c | (uint16_t)0x0F << 8; // white text
        term_col++;
    }

    if (term_col >= VGA_WIDTH) {
        term_col = 0;
        term_row++;
    }

    if (term_row >= VGA_HEIGHT) {
        // Scroll up
        for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
            video_memory[i] = video_memory[i + VGA_WIDTH];
        }
        // Clear last line
        for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
            video_memory[i] = (uint16_t)' ' | (uint16_t)0x0F << 8;
        }
        term_row = VGA_HEIGHT - 1;
    }

    update_cursor(term_col, term_row);
}

void terminal_writestring(const char* s) {
    for (int i = 0; s[i] != '\0'; i++) terminal_putchar(s[i]);
}

// Прототипы
void init_gdt();
void idt_init();
void irq_install();
extern void ata_init();
extern int ata_detect_disks();
extern void shell_main();

// Set VGA text mode (80x25)
void vga_set_text_mode() {
    __asm__ volatile("mov $0x03, %%ah; int $0x10" : : : "ah"); // BIOS set mode 3
}

void kmain(multiboot_info_t* mb_info, uint32_t magic) {
    terminal_initialize();
    init_gdt();
    idt_init();
    irq_install();
    ata_init();

    int disk_count = ata_detect_disks();
    terminal_writestring("Lakos OS v0.3 Booted! Disks: ");
    // simple print
    terminal_putchar('0' + disk_count);
    terminal_writestring("\n");

    __asm__ volatile("sti");
    shell_main();

    while(1) { __asm__ volatile("hlt"); }
}