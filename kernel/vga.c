/*
 * Lakos OS
 * Copyright (c) 2026 lakladon
 * Created: January 10, 2026
 */

#include <stdint.h>

int abs(int x) { return x < 0 ? -x : x; }

// VGA registers
#define VGA_MISC_WRITE 0x3C2
#define VGA_SEQ_INDEX 0x3C4
#define VGA_SEQ_DATA 0x3C5
#define VGA_CRTC_INDEX 0x3D4
#define VGA_CRTC_DATA 0x3D5
#define VGA_GC_INDEX 0x3CE
#define VGA_GC_DATA 0x3CF
#define VGA_AC_INDEX 0x3C0
#define VGA_AC_WRITE 0x3C0
#define VGA_AC_READ 0x3C1
#define VGA_INSTAT_READ 0x3DA

// Video memory
#define VGA_MEMORY 0xA0000

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void vga_set_mode_13h() {
    // Set misc register
    outb(VGA_MISC_WRITE, 0x63);

    // Sequencer
    outb(VGA_SEQ_INDEX, 0);
    outb(VGA_SEQ_DATA, 0x03);
    outb(VGA_SEQ_INDEX, 1);
    outb(VGA_SEQ_DATA, 0x01);
    outb(VGA_SEQ_INDEX, 2);
    outb(VGA_SEQ_DATA, 0x0F);
    outb(VGA_SEQ_INDEX, 3);
    outb(VGA_SEQ_DATA, 0x00);
    outb(VGA_SEQ_INDEX, 4);
    outb(VGA_SEQ_DATA, 0x06);

    // CRTC
    outb(VGA_CRTC_INDEX, 0);
    outb(VGA_CRTC_DATA, 0x5F);
    outb(VGA_CRTC_INDEX, 1);
    outb(VGA_CRTC_DATA, 0x4F);
    outb(VGA_CRTC_INDEX, 2);
    outb(VGA_CRTC_DATA, 0x50);
    outb(VGA_CRTC_INDEX, 3);
    outb(VGA_CRTC_DATA, 0x82);
    outb(VGA_CRTC_INDEX, 4);
    outb(VGA_CRTC_DATA, 0x54);
    outb(VGA_CRTC_INDEX, 5);
    outb(VGA_CRTC_DATA, 0x80);
    outb(VGA_CRTC_INDEX, 6);
    outb(VGA_CRTC_DATA, 0xBF);
    outb(VGA_CRTC_INDEX, 7);
    outb(VGA_CRTC_DATA, 0x1F);
    outb(VGA_CRTC_INDEX, 8);
    outb(VGA_CRTC_DATA, 0x00);
    outb(VGA_CRTC_INDEX, 9);
    outb(VGA_CRTC_DATA, 0x41);
    outb(VGA_CRTC_INDEX, 10);
    outb(VGA_CRTC_DATA, 0x00);
    outb(VGA_CRTC_INDEX, 11);
    outb(VGA_CRTC_DATA, 0x00);
    outb(VGA_CRTC_INDEX, 12);
    outb(VGA_CRTC_DATA, 0x00);
    outb(VGA_CRTC_INDEX, 13);
    outb(VGA_CRTC_DATA, 0x00);
    outb(VGA_CRTC_INDEX, 14);
    outb(VGA_CRTC_DATA, 0x00);
    outb(VGA_CRTC_INDEX, 15);
    outb(VGA_CRTC_DATA, 0x00);
    outb(VGA_CRTC_INDEX, 16);
    outb(VGA_CRTC_DATA, 0x9C);
    outb(VGA_CRTC_INDEX, 17);
    outb(VGA_CRTC_DATA, 0x8E);
    outb(VGA_CRTC_INDEX, 18);
    outb(VGA_CRTC_DATA, 0x8F);
    outb(VGA_CRTC_INDEX, 19);
    outb(VGA_CRTC_DATA, 0x28);
    outb(VGA_CRTC_INDEX, 20);
    outb(VGA_CRTC_DATA, 0x1F);
    outb(VGA_CRTC_INDEX, 21);
    outb(VGA_CRTC_DATA, 0x96);
    outb(VGA_CRTC_INDEX, 22);
    outb(VGA_CRTC_DATA, 0xB9);
    outb(VGA_CRTC_INDEX, 23);
    outb(VGA_CRTC_DATA, 0xA3);
    outb(VGA_CRTC_INDEX, 24);
    outb(VGA_CRTC_DATA, 0xFF);

    // GC
    outb(VGA_GC_INDEX, 0);
    outb(VGA_GC_DATA, 0x00);
    outb(VGA_GC_INDEX, 1);
    outb(VGA_GC_DATA, 0x00);
    outb(VGA_GC_INDEX, 2);
    outb(VGA_GC_DATA, 0x00);
    outb(VGA_GC_INDEX, 3);
    outb(VGA_GC_DATA, 0x00);
    outb(VGA_GC_INDEX, 4);
    outb(VGA_GC_DATA, 0x00);
    outb(VGA_GC_INDEX, 5);
    outb(VGA_GC_DATA, 0x40);
    outb(VGA_GC_INDEX, 6);
    outb(VGA_GC_DATA, 0x05);
    outb(VGA_GC_INDEX, 7);
    outb(VGA_GC_DATA, 0x0F);
    outb(VGA_GC_INDEX, 8);
    outb(VGA_GC_DATA, 0xFF);

    // AC
    uint8_t ac_data[21] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
                           0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
                           0x0C, 0x00, 0x0F, 0x08, 0x00};
    for (int i = 0; i < 21; i++) {
        inb(VGA_INSTAT_READ); // reset flip-flop
        outb(VGA_AC_INDEX, i);
        outb(VGA_AC_WRITE, ac_data[i]);
    }
    inb(VGA_INSTAT_READ);
    outb(VGA_AC_INDEX, 0x20); // enable
}

void vga_put_pixel(int x, int y, uint8_t color) {
    uint8_t* vga = (uint8_t*)VGA_MEMORY;
    vga[y * 320 + x] = color;
}

void vga_clear_screen(uint8_t color) {
    uint8_t* vga = (uint8_t*)VGA_MEMORY;
    for (int i = 0; i < 320 * 200; i++) {
        vga[i] = color;
    }
}

void vga_draw_line(int x1, int y1, int x2, int y2, uint8_t color) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = (abs(dx) > abs(dy)) ? abs(dx) : abs(dy);
    float xinc = dx / (float)steps;
    float yinc = dy / (float)steps;
    float x = x1;
    float y = y1;
    for (int i = 0; i <= steps; i++) {
        vga_put_pixel((int)x, (int)y, color);
        x += xinc;
        y += yinc;
    }
}

void vga_draw_rectangle(int x, int y, int w, int h, uint8_t color) {
    for (int i = x; i < x + w; i++) {
        vga_put_pixel(i, y, color);
        vga_put_pixel(i, y + h - 1, color);
    }
    for (int i = y; i < y + h; i++) {
        vga_put_pixel(x, i, color);
        vga_put_pixel(x + w - 1, i, color);
    }
}

void vga_fill_rectangle(int x, int y, int w, int h, uint8_t color) {
    for (int i = y; i < y + h; i++) {
        for (int j = x; j < x + w; j++) {
            vga_put_pixel(j, i, color);
        }
    }
}

// External font functions from font.c
extern const uint8_t* font_get_bitmap(uint16_t unicode);
extern uint16_t utf8_to_unicode(const char** str);
extern int font_get_width();
extern int font_get_height();

void vga_draw_char(int x, int y, char c, uint8_t color) {
    // Use extended font for ASCII
    const uint8_t* bitmap = font_get_bitmap((uint8_t)c);
    if (!bitmap) return;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (bitmap[i] & (1 << (7 - j))) {
                vga_put_pixel(x + j, y + i, color);
            }
        }
    }
}

// Draw a Unicode character using font bitmap
void vga_draw_char_unicode(int x, int y, uint16_t unicode, uint8_t color) {
    const uint8_t* bitmap = font_get_bitmap(unicode);
    if (!bitmap) return;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (bitmap[i] & (1 << (7 - j))) {
                vga_put_pixel(x + j, y + i, color);
            }
        }
    }
}

void vga_draw_text(int x, int y, const char* text, uint8_t color) {
    int cx = x;
    const char* ptr = text;
    while (*ptr) {
        if (*ptr == '\n') {
            y += 8;
            cx = x;
            ptr++;
        } else {
            // Decode UTF-8 character
            uint16_t unicode = utf8_to_unicode(&ptr);
            if (unicode == 0) break;
            vga_draw_char_unicode(cx, y, unicode, color);
            cx += 8;
        }
    }
}

// Draw text with UTF-8 support (alias for vga_draw_text)
void vga_draw_text_utf8(int x, int y, const char* text, uint8_t color) {
    vga_draw_text(x, y, text, color);
}
