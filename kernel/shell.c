#include <stdint.h>
#include <stddef.h>

// Объявляем внешние функции из kernel.c и других модулей
extern void terminal_putchar(char c);
extern void terminal_writestring(const char* data);
extern size_t strlen(const char* str);
extern int strcmp(const char* s1, const char* s2);
extern void* tar_lookup(void* archive, const char* filename);
extern void tar_list_files(void* archive);
extern void* initrd_location; // Переменная из kernel.c
// Добавь в конец kernel/shell.c

char shell_buffer[256];
int shell_ptr = 0;

void shell_execute() {
    if (strlen(shell_buffer) == 0) return;

    if (strcmp(shell_buffer, "help") == 0) {
        terminal_writestring("Available commands: help, ls, clear\n");
    } 
    else if (strcmp(shell_buffer, "ls") == 0) {
        tar_list_files(initrd_location);
    } 
    else {
        void* file = tar_lookup(initrd_location, shell_buffer);
        if (file) {
            void (*entry)() = (void (*)())file;
            entry();
        } else {
            terminal_writestring("Command not found: ");
            terminal_writestring(shell_buffer);
            terminal_putchar('\n');
        }
    }
}

void shell_feed_char(char c) {
    if (c == '\n') {
        shell_buffer[shell_ptr] = '\0';
        terminal_putchar('\n');
        shell_execute();
        shell_ptr = 0;
        terminal_writestring("Lakos> ");
    } else {
        if (shell_ptr < 255) {
            shell_buffer[shell_ptr++] = c;
            terminal_putchar(c);
        }
    }
}

void shell_main() {
    terminal_writestring("Lakos> ");
    while(1) {
        __asm__("hlt");
    }
}
// Добавь в конец kernel/shell.c
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}
