#include <stdint.h>

extern void terminal_writestring(const char* data);
extern void terminal_putchar(char c);
extern volatile char last_key; // Теперь это просто ссылка на isr.c
extern void start_gui();

int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) return *s1 - *s2;
        s1++; s2++;
    }
    return *s1 - *s2;
}

char command_buffer[256];
int buffer_index = 0;

void shell_feed_char(char c) {
    last_key = c;
}

void process_command(char* cmd) {
    if (strcmp(cmd, "intref") == 0) {
        start_gui();
    } else {
        terminal_writestring("Unknown command: ");
        terminal_writestring(cmd);
        terminal_putchar('\n');
    }
}

void shell_main() {
    terminal_writestring("Shell loaded. Type 'intref' to start GUI!\n");
    while(1) {
        terminal_writestring("Lakos> ");
        buffer_index = 0;
        while(1) {
            last_key = 0;
            while(last_key == 0) {
                __asm__ volatile("hlt");
            }
            char c = last_key;
            if (c == '\n' || c == '\r') {
                command_buffer[buffer_index] = '\0';
                terminal_putchar('\n');
                process_command(command_buffer);
                break;
            } else if (c == '\b') {
                if (buffer_index > 0) {
                    buffer_index--;
                    terminal_writestring("\b \b");
                }
            } else if (c >= 32 && c <= 126) { // printable
                command_buffer[buffer_index++] = c;
                terminal_putchar(c);
            }
        }
    }
}