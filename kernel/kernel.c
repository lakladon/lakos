

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

uint8_t bcd_to_bin(uint8_t bcd) {
    return (bcd / 16) * 10 + (bcd % 16);
}

void rtc_get_time(int* hours, int* minutes, int* seconds) {

    while (rtc_is_updating());

    *seconds = bcd_to_bin(rtc_read(0x00));
    *minutes = bcd_to_bin(rtc_read(0x02));
    *hours = bcd_to_bin(rtc_read(0x04));
}

void kmain(multiboot_info_t* mb_info, uint32_t magic);

const multiboot_header_t __attribute__((section(".multiboot"))) header = {
    .magic = MULTIBOOT_HEADER_MAGIC,
    .flags = 0,
    .checksum = -(MULTIBOOT_HEADER_MAGIC + 0)
};

#define VIDEO_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define VGA_AC_INDEX 0x3C0
#define VGA_AC_WRITE 0x3C0
#define VGA_AC_READ 0x3C1
#define VGA_INSTAT_READ 0x3DA

uint16_t* video_memory = (uint16_t*)VIDEO_MEMORY;

int term_col = 0;
int term_row = 0;
uint8_t current_attr = 0x0F; 
uint8_t current_bg_index = 0; 
int cursor_visible = 1;

void terminal_update_cursor() {
    uint16_t pos = term_row * VGA_WIDTH + term_col;
    outb(0x3D4, 0x0F); 
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E); 
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void terminal_enable_cursor() {
    outb(0x3D4, 0x0A); 
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 0); 
    outb(0x3D4, 0x0B); 
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15); 
    cursor_visible = 1;
}

void terminal_disable_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20); 
    cursor_visible = 0;
}

void terminal_move_cursor(int col) {
    if (col >= 0 && col < VGA_WIDTH) {
        term_col = col;
        terminal_update_cursor();
    }
}

int terminal_get_cursor_col() {
    return term_col;
}

int terminal_get_cursor_row() {
    return term_row;
}

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

void vga_set_palette_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {

    r = r >> 2;  
    g = g >> 2;
    b = b >> 2;

    outb(0x3C8, index);  
    outb(0x3C9, r);      
    outb(0x3C9, g);      
    outb(0x3C9, b);      
}

void terminal_set_background(uint8_t bg_color) {

    current_bg_index = bg_color & 0x0F;

    current_attr = (current_attr & 0x0F) | ((bg_color & 0x0F) << 4);

    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        uint16_t old_char = video_memory[i];
        char c = (char)(old_char & 0xFF);
        video_memory[i] = (uint16_t)c | (uint16_t)current_attr << 8;
    }
}

static void vga_disable_blink() {

    inb(VGA_INSTAT_READ);  
    outb(VGA_AC_INDEX, 0x10);
    uint8_t mode = inb(VGA_AC_READ);

    outb(VGA_AC_INDEX, 0x10);
    outb(VGA_AC_WRITE, mode | 0x08);
}

void terminal_set_background_rgb(uint8_t r, uint8_t g, uint8_t b) {

    vga_disable_blink();

    vga_set_palette_color(8, r, g, b);

    current_bg_index = 8;

    current_attr = (15 & 0x0F) | ((8 & 0x0F) << 4);  

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

void terminal_putchar_at(int col, int row, char c) {
    if (row >= 0 && row < VGA_HEIGHT && col >= 0 && col < VGA_WIDTH) {
        video_memory[row * VGA_WIDTH + col] = (uint16_t)c | (uint16_t)current_attr << 8;
    }
}

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
                if (code == 0) current_attr = 0x0F | (current_bg_index << 4); 
                else if (code == 31) current_attr = 0x04 | (current_bg_index << 4); 
                else if (code == 32) current_attr = 0x02 | (current_bg_index << 4); 
                else if (code == 33) current_attr = 0x0E | (current_bg_index << 4); 
                else if (code == 36) current_attr = 0x03 | (current_bg_index << 4); 
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

void terminal_display_time() {
    int hours, minutes, seconds;
    rtc_get_time(&hours, &minutes, &seconds);

    int saved_col = term_col;
    int saved_row = term_row;
    uint8_t saved_attr = current_attr;

    int time_col = 71;
    int time_row = 0;

    current_attr = (0x0B & 0x0F) | (current_bg_index << 4); 

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

    for (int i = 0; i < 8; i++) {
        video_memory[time_row * VGA_WIDTH + time_col + i] = 
            (uint16_t)time_str[i] | (uint16_t)current_attr << 8;
    }

    term_col = saved_col;
    term_row = saved_row;
    current_attr = saved_attr;
}

void init_gdt();
void idt_init();
void irq_install();
extern void ata_init();
extern int ata_detect_disks();
extern void shell_main();
extern void init_kernel_commands();

void* tar_archive = 0;
extern char _binary_modules_tar_start[];

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