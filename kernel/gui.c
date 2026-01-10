#include <stdint.h>

extern void vga_set_mode_13h();
extern void vga_clear_screen(uint8_t color);
extern void vga_fill_rectangle(int x, int y, int w, int h, uint8_t color);
extern void vga_draw_text(int x, int y, const char* text, uint8_t color);
extern volatile int mouse_x;
extern volatile int mouse_y;

void gui_init() {
    vga_set_mode_13h();
    // Desktop background blue
    vga_clear_screen(1); // blue
    // Taskbar at bottom
    vga_fill_rectangle(0, 180, 320, 20, 7); // gray
    vga_draw_text(10, 185, "Lakos GUI", 0); // black
}

void gui_draw_cursor() {
    // Simple cursor: small white square
    vga_fill_rectangle(mouse_x, mouse_y, 5, 5, 15);
}

void gui_main() {
    gui_init();
    while(1) {
        // Clear old cursor
        vga_fill_rectangle(mouse_x, mouse_y, 5, 5, 1); // redraw background
        // Draw new cursor
        gui_draw_cursor();
        // Handle input, but for now loop
        __asm__ volatile("hlt");
    }
}

void start_gui() {
    gui_main();
}