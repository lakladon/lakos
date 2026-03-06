/*
 * Lakos OS
 * Copyright (c) 2026 lakladon
 * Created: February 23, 2026
 */

#include "include/multiboot.h"
#include <stdint.h>
#include <stddef.h>
#include "include/version.h"
#include "include/lib.h"

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// RTC functions for real-time clock
uint8_t rtc_read(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

void rtc_write(uint8_t reg, uint8_t val) {
    outb(0x70, reg);
    outb(0x71, val);
}

int rtc_is_updating() {
    outb(0x70, 0x0A);
    return (inb(0x71) & 0x80);
}

// Convert BCD to binary
uint8_t bcd_to_bin(uint8_t bcd) {
    return (bcd / 16) * 10 + (bcd % 16);
}

// Get current time from RTC
void rtc_get_time(int* hours, int* minutes, int* seconds) {
    // Wait if RTC is updating
    while (rtc_is_updating());
    
    *seconds = bcd_to_bin(rtc_read(0x00));
    *minutes = bcd_to_bin(rtc_read(0x02));
    *hours = bcd_to_bin(rtc_read(0x04));
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
uint8_t current_bg_index = 0; // current background palette index (0-15)
int cursor_visible = 1;

// Update hardware cursor position
void terminal_update_cursor() {
    uint16_t pos = term_row * VGA_WIDTH + term_col;
    outb(0x3D4, 0x0F); // Cursor location low
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E); // Cursor location high
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

// Enable blinking cursor
void terminal_enable_cursor() {
    outb(0x3D4, 0x0A); // Cursor start
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 0); // Start at scanline 0
    outb(0x3D4, 0x0B); // Cursor end
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15); // End at scanline 15
    cursor_visible = 1;
}

// Disable cursor
void terminal_disable_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20); // Disable cursor
    cursor_visible = 0;
}

// Move cursor to specific column (keeping current row)
void terminal_move_cursor(int col) {
    if (col >= 0 && col < VGA_WIDTH) {
        term_col = col;
        terminal_update_cursor();
    }
}

// Get current cursor column
int terminal_get_cursor_col() {
    return term_col;
}

// Get current cursor row
int terminal_get_cursor_row() {
    return term_row;
}

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
    terminal_enable_cursor();
    terminal_update_cursor();
}

// Set VGA DAC palette color (index 0-15, RGB values 0-63 each)
void vga_set_palette_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    // VGA DAC uses 6-bit values (0-63), so we need to scale from 0-255 to 0-63
    r = r >> 2;  // Scale 0-255 to 0-63
    g = g >> 2;
    b = b >> 2;
    
    outb(0x3C8, index);  // DAC address write register
    outb(0x3C9, r);      // Red component
    outb(0x3C9, g);      // Green component
    outb(0x3C9, b);      // Blue component
}

// Set background color and redraw screen
void terminal_set_background(uint8_t bg_color) {
    // Store the background index
    current_bg_index = bg_color & 0x0F;
    
    // Update current attribute: keep foreground (low 4 bits), set background (high 4 bits)
    current_attr = (current_attr & 0x0F) | ((bg_color & 0x0F) << 4);
    
    // Redraw entire screen with new background color
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        uint16_t old_char = video_memory[i];
        char c = (char)(old_char & 0xFF);
        video_memory[i] = (uint16_t)c | (uint16_t)current_attr << 8;
    }
}

// Disable blink mode to allow bright backgrounds (indices 8-15)
static void vga_disable_blink() {
    // Read Attribute Controller Mode Control Register (index 0x10)
    inb(VGA_INSTAT_READ);  // Reset flip-flop
    outb(VGA_AC_INDEX, 0x10);
    uint8_t mode = inb(VGA_AC_READ);
    // Set bit 3 to enable background intensity (disable blink)
    outb(VGA_AC_INDEX, 0x10);
    outb(VGA_AC_WRITE, mode | 0x08);
}

// Set exact RGB background color by reprogramming VGA palette
void terminal_set_background_rgb(uint8_t r, uint8_t g, uint8_t b) {
    // Disable blink mode to allow using palette index 8 for background
    vga_disable_blink();
    
    // Use palette index 8 for custom background color
    vga_set_palette_color(8, r, g, b);
    
    // Store the background index
    current_bg_index = 8;
    
    // Set background to index 8 (which now has our custom color)
    // Keep foreground as white (index 15) for visibility
    current_attr = (15 & 0x0F) | ((8 & 0x0F) << 4);  // foreground=white, background=custom(index 8)
    
    // Redraw entire screen with new background color
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        uint16_t old_char = video_memory[i];
        char c = (char)(old_char & 0xFF);
        video_memory[i] = (uint16_t)c | (uint16_t)current_attr << 8;
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
    
    terminal_update_cursor();
}

// Write a character at specific position without moving cursor
void terminal_putchar_at(int col, int row, char c) {
    if (row >= 0 && row < VGA_HEIGHT && col >= 0 && col < VGA_WIDTH) {
        video_memory[row * VGA_WIDTH + col] = (uint16_t)c | (uint16_t)current_attr << 8;
    }
}

// Get character at specific position
char terminal_getchar_at(int col, int row) {
    if (row >= 0 && row < VGA_HEIGHT && col >= 0 && col < VGA_WIDTH) {
        return (char)(video_memory[row * VGA_WIDTH + col] & 0xFF);
    }
    return ' ';
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
                if (code == 0) current_attr = 0x0F | (current_bg_index << 4); // reset to white on current background
                else if (code == 31) current_attr = 0x04 | (current_bg_index << 4); // red on current background
                else if (code == 32) current_attr = 0x02 | (current_bg_index << 4); // green on current background
                else if (code == 33) current_attr = 0x0E | (current_bg_index << 4); // yellow on current background
                else if (code == 36) current_attr = 0x03 | (current_bg_index << 4); // cyan on current background
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

// Display time in top right corner
void terminal_display_time() {
    int hours, minutes, seconds;
    rtc_get_time(&hours, &minutes, &seconds);
    
    // Save current cursor position
    int saved_col = term_col;
    int saved_row = term_row;
    uint8_t saved_attr = current_attr;
    
    // Position at top right (column 71 for HH:MM:SS format)
    int time_col = 71;
    int time_row = 0;
    
    // Use cyan foreground with current background color
    current_attr = (0x0B & 0x0F) | (current_bg_index << 4); // cyan on current background
    
    // Format time string HH:MM:SS
    char time_str[9];
    time_str[0] = '0' + (hours / 10);
    time_str[1] = '0' + (hours % 10);
    time_str[2] = ':';
    time_str[3] = '0' + (minutes / 10);
    time_str[4] = '0' + (minutes % 10);
    time_str[5] = ':';
    time_str[6] = '0' + (seconds / 10);
    time_str[7] = '0' + (seconds % 10);
    time_str[8] = '\0';
    
    // Write time at top right
    for (int i = 0; i < 8; i++) {
        video_memory[time_row * VGA_WIDTH + time_col + i] = 
            (uint16_t)time_str[i] | (uint16_t)current_attr << 8;
    }
    
    // Restore cursor position and attribute
    term_col = saved_col;
    term_row = saved_row;
    current_attr = saved_attr;
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