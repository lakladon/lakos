#include "include/multiboot.h"
#include <stdint.h>
#include <stddef.h>
#include "include/version.h"
#include "include/lib.h"

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

// Text mode terminal
#define VIDEO_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

uint16_t* video_memory = (uint16_t*)VIDEO_MEMORY;

int term_col = 0;
int term_row = 0;
uint8_t current_attr = 0x0F; // white on black

// Optional terminal output capture (used for shell pipes).
static int term_capture_enabled = 0;
static char* term_capture_buffer = 0;
static int term_capture_size = 0;
static int term_capture_pos = 0;

void terminal_capture_begin(char* buffer, int size) {
    term_capture_buffer = buffer;
    term_capture_size = size;
    term_capture_pos = 0;
    term_capture_enabled = (buffer && size > 0) ? 1 : 0;
    if (term_capture_enabled) {
        term_capture_buffer[0] = '\0';
    }
}

void terminal_capture_end() {
    if (term_capture_enabled && term_capture_buffer && term_capture_size > 0) {
        if (term_capture_pos >= term_capture_size) {
            term_capture_pos = term_capture_size - 1;
        }
        term_capture_buffer[term_capture_pos] = '\0';
    }
    term_capture_enabled = 0;
    term_capture_buffer = 0;
    term_capture_size = 0;
    term_capture_pos = 0;
}

void terminal_initialize() {
    term_col = 0;
    term_row = 0;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        video_memory[i] = (uint16_t)' ' | (uint16_t)0x0F << 8;
    }
}

void terminal_putchar(char c) {
    if (term_capture_enabled && term_capture_buffer && term_capture_size > 0) {
        if (c == '\b') {
            if (term_capture_pos > 0) term_capture_pos--;
        } else if (term_capture_pos < term_capture_size - 1) {
            term_capture_buffer[term_capture_pos++] = c;
        }
        term_capture_buffer[term_capture_pos] = '\0';
        return;
    }

    if (c == '\n') {
        term_col = 0;
        term_row++;
    } else if (c == '\b') {
        if (term_col > 0) {
            term_col--;
            video_memory[term_row * VGA_WIDTH + term_col] = (uint16_t)' ' | (uint16_t)current_attr << 8;
        }
    } else {
        video_memory[term_row * VGA_WIDTH + term_col] = (uint16_t)c | (uint16_t)current_attr << 8;
        term_col++;
    }

    if (term_col >= VGA_WIDTH) {
        term_col = 0;
        term_row++;
    }

    if (term_row >= VGA_HEIGHT) {
        for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
            video_memory[i] = video_memory[i + VGA_WIDTH];
        }
        for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
            video_memory[i] = (uint16_t)' ' | (uint16_t)current_attr << 8;
        }
        term_row = VGA_HEIGHT - 1;
    }
}

void terminal_writestring(const char* s) {
    while (*s) {
        if (*s == '\033' && *(s+1) == '[') {
            s += 2;
            int code = 0;
            while (*s >= '0' && *s <= '9') {
                code = code * 10 + (*s - '0');
                s++;
            }
            if (*s == 'm') {
                s++;
                if (code == 0) current_attr = 0x0F;
                else if (code == 31) current_attr = 0x04;
                else if (code == 32) current_attr = 0x02;
                else if (code == 33) current_attr = 0x0E;
                else if (code == 36) current_attr = 0x03;
            } else {
                while (*s && *s != 'm') s++;
                if (*s == 'm') s++;
            }
        } else {
            terminal_putchar(*s);
            s++;
        }
    }
}

// Prototypes
void init_gdt();
void idt_init();
void irq_install();
extern void ata_init();
extern int ata_detect_disks();
extern void shell_main();
extern void init_kernel_commands();

void* tar_archive = 0;
extern char _binary_modules_tar_start[];

// Simple boot
void kmain(multiboot_info_t* mb_info, uint32_t magic) {
    (void)magic;
    (void)mb_info;
    
    terminal_initialize();

    terminal_writestring("Lakos OS v");
    terminal_writestring(KERNEL_VERSION);
    terminal_writestring("\n\n");

    init_gdt();
    idt_init();
    irq_install();
    
    tar_archive = (void*)&_binary_modules_tar_start;
    
    ata_init();
    ata_detect_disks();

    __asm__ volatile("sti");
    init_kernel_commands();
    shell_main();

    while(1) { __asm__ volatile("hlt"); }
}