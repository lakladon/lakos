#include <stdint.h>
#include <io.h>

extern void vga_set_mode_13h();
extern void vga_clear_screen(uint8_t color);
extern void vga_fill_rectangle(int x, int y, int w, int h, uint8_t color);
extern void vga_draw_text(int x, int y, const char* text, uint8_t color);
extern volatile int mouse_x;
extern volatile int mouse_y;
extern volatile uint8_t mouse_buttons;

void gui_init() {
    vga_set_mode_13h();
    // Display a 256-color gradient image
    for (int y = 0; y < 200; y++) {
        for (int x = 0; x < 320; x++) {
            uint8_t color = (x + y) % 256;
            vga_put_pixel(x, y, color);
        }
    }
}

void gui_draw_cursor() {
    // Simple cursor: small white square
    vga_fill_rectangle(mouse_x, mouse_y, 5, 5, 15);
}

void gui_main() {
    gui_init();
    // Minimal GUI: just display the image and halt
    while(1) {
        __asm__ volatile("hlt");
    }
}

void start_gui() {
    gui_main();
}