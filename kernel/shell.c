#include <stdint.h>

extern void terminal_writestring(const char* data);
extern volatile char last_key; // Берем из isr.c

void shell_main() {
    terminal_writestring("Shell loaded. Type something!\n");

    while(1) {
        terminal_writestring("\nLakos> ");
        
        // Ждем нажатия клавиши (цикл ожидания)
        last_key = 0;
        while(last_key == 0) {
            // Процессор отдыхает, пока не прилетит прерывание
            __asm__ volatile("hlt");
        }
        
        // Когда клавиша нажата, цикл пойдет на следующую итерацию
        // и снова выведет Lakos> только ПОСЛЕ ввода.
    }
}