#include <stdint.h>
#include <io.h>
extern void vga_set_mode_13h();
extern void vga_clear_screen(uint8_t color);
extern void vga_fill_rectangle(int x, int y, int w, int h, uint8_t color);
extern void vga_draw_rectangle(int x, int y, int w, int h, uint8_t color);
extern void vga_draw_text(int x, int y, const char* text, uint8_t color);
extern volatile int mouse_x;
extern volatile int mouse_y;
extern volatile uint8_t mouse_buttons;
void gui_init() {
    vga_set_mode_13h();
    vga_clear_screen(0); 
    vga_fill_rectangle(0, 190, 320, 10, 2); 
    vga_draw_text(5, 192, "[Workspace 1] | No app | 00:00 | 0.0Mb/s 0.0Mb/s", 10); 
    vga_fill_rectangle(0, 170, 320, 20, 7); 
    vga_draw_text(10, 175, "LAKOS ATLAS", 0); 
    vga_fill_rectangle(250, 172, 60, 16, 8); 
    vga_draw_text(255, 175, "Start", 0); 
}
void gui_draw_cursor() {
    vga_fill_rectangle(mouse_x, mouse_y, 5, 5, 15);
}
void gui_main() {
    gui_init();
    static int window_open = 0;
    static uint8_t prev_buttons = 0;
    static int simulate_click = 0;
    while(1) {
        vga_fill_rectangle(mouse_x, mouse_y, 5, 5, 0); 
        if (inb(0x64) & 0x1) {
            uint8_t scancode = inb(0x60);
            if (!(scancode & 0x80)) { 
                if (scancode == 0x4B) mouse_x -= 5; 
                if (scancode == 0x4D) mouse_x += 5; 
                if (scancode == 0x48) mouse_y -= 5; 
                if (scancode == 0x50) mouse_y += 5; 
                if (scancode == 0x39) simulate_click = 1; 
                if (mouse_x < 0) mouse_x = 0;
                if (mouse_x >= 320) mouse_x = 319;
                if (mouse_y < 0) mouse_y = 0;
                if (mouse_y >= 200) mouse_y = 199;
            }
        }
        int click = (mouse_buttons & 1) || simulate_click;
        if (click && !(prev_buttons & 1) && !simulate_click) { 
            if (mouse_x >= 250 && mouse_x < 310 && mouse_y >= 172 && mouse_y < 188) {
                window_open = !window_open;
            }
        }
        if (simulate_click) simulate_click = 0;
        prev_buttons = mouse_buttons;
        vga_fill_rectangle(0, 170, 320, 20, 7); 
        vga_draw_text(10, 175, "LAKOS ATLAS", 0); 
        vga_fill_rectangle(250, 172, 60, 16, 8); 
        vga_draw_text(255, 175, "Start", 0); 
        vga_fill_rectangle(0, 190, 320, 10, 2); 
        vga_draw_text(5, 192, "[Workspace 1] | No app | 00:00 | 0.0Mb/s 0.0Mb/s", 10); 
        if (window_open) {
            vga_fill_rectangle(100, 50, 120, 80, 15); 
            vga_draw_rectangle(100, 50, 120, 80, 10); 
            vga_draw_text(110, 60, "Hello World", 0); 
        }
        gui_draw_cursor();
        __asm__ volatile("hlt");
    }
}
void start_gui() {
    gui_main();
}