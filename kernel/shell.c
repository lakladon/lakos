#include <stdint.h>

extern void terminal_writestring(const char* data);
extern void terminal_putchar(char c);

volatile char last_key = 0;

// ТА САМАЯ ФУНКЦИЯ, которой не хватало линковщику
void shell_feed_char(char c) {
    last_key = c;
}

void shell_main() {
    while(1) {
        terminal_writestring("Lakos> ");
        last_key = 0;
        while(last_key == 0) {
            __asm__ volatile("hlt");
        }
        // После нажатия клавиши цикл начнется заново
    }
}