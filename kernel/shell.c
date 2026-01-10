#include <stdint.h>

extern void terminal_writestring(const char* data);
extern void terminal_putchar(char c);
extern volatile char last_key; // Теперь это просто ссылка на isr.c

void shell_feed_char(char c) {
    last_key = c;
}

void shell_main() {
    terminal_writestring("Shell loaded. Type something!\n");
    while(1) {
        terminal_writestring("Lakos> ");
        last_key = 0;
        while(last_key == 0) {
            __asm__ volatile("hlt");
        }
        // После того как прерывание запишет символ в last_key, цикл продолжится
    }
}