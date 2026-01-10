#include <stdint.h>
#include <io.h>

extern void terminal_writestring(const char* s);
extern void terminal_putchar(char c);
extern void terminal_initialize();

unsigned char kbd_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0
};

unsigned char kbd_map_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0
};

static int shift_pressed = 0;

static char shell_buf[256];
static int shell_ptr = 0;

// Функция сравнения строк (без нее команды не найти)
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// Твоя функция поиска команд и бинарников
void writeUSERterminal(const char* input) {
    if (strcmp(input, "help") == 0) {
        terminal_writestring("Lakos OS Commands: help, cls, ver, ls, bin/test\n");
    } 
    else if (strcmp(input, "cls") == 0) {
        terminal_initialize();
    } 
    else if (strcmp(input, "ver") == 0) {
        terminal_writestring("Lakos OS v0.3.7 [Kernel Mode]\n");
    }
    else if (strcmp(input, "ls") == 0) {
        terminal_writestring("bin/  dev/  home/\n");
    }
    else if (strcmp(input, "bin/test") == 0) {
        terminal_writestring("Running binary from virtual /bin/...\nDone.\n");
    }
    else {
        terminal_writestring("Error: command '");
        terminal_writestring(input);
        terminal_writestring("' not found.\n");
    }
}

// Эту функцию вызывает драйвер клавиатуры
void shell_handle_key(char c) {
    if (c == '\n') {
        terminal_putchar('\n');
        shell_buf[shell_ptr] = '\0';
        
        if (shell_ptr > 0) {
            writeUSERterminal(shell_buf);
        }
        
        terminal_writestring("Lakos> ");
        shell_ptr = 0;
    } 
    else if (c == '\b') {
        if (shell_ptr > 0) {
            shell_ptr--;
            terminal_putchar('\b');
            terminal_putchar(' ');
            terminal_putchar('\b');
        }
    } 
    else if (shell_ptr < 255) {
        shell_buf[shell_ptr++] = c;
        terminal_putchar(c);
    }
}

// ГЛАВНАЯ ФУНКЦИЯ ШЕЛЛА
void shell_main() {
    terminal_writestring("\n--- Lakos Shell Started ---\nLakos> ");
    while(1) {
        if (inb(0x64) & 0x1) {
            uint8_t scancode = inb(0x60);
            if (scancode == 42 || scancode == 54) {
                shift_pressed = !(scancode & 0x80);
            } else if (!(scancode & 0x80)) {
                char c = shift_pressed ? kbd_map_shift[scancode] : kbd_map[scancode];
                if (c != 0) shell_handle_key(c);
            }
        }
    }
}