#include <stdint.h>
#include <io.h>
#include "include/users.h"
#include "include/version.h"
#include "include/lib.h"
#include "include/commands.h"

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
static int caps_locked = 0;

void read_line(char* buffer, int max, int echo) {
    int ptr = 0;
    while (1) {
        if (inb(0x64) & 0x1) {
            uint8_t scancode = inb(0x60);
            if ((scancode & 0x7F) == 42 || (scancode & 0x7F) == 54) {
                shift_pressed = !(scancode & 0x80);
            } else if (scancode == 58) {
                if (!(scancode & 0x80)) caps_locked = !caps_locked;
            } else if (!(scancode & 0x80)) {
                int is_letter = (scancode >= 16 && scancode <= 25) || (scancode >= 30 && scancode <= 38) || (scancode >= 44 && scancode <= 50);
                int uppercase = shift_pressed || (caps_locked && is_letter);
                char c = uppercase ? kbd_map_shift[scancode] : kbd_map[scancode];
                if (c == '\n') {
                    buffer[ptr] = '\0';
                    if (echo) terminal_putchar('\n');
                    return;
                } else if (c == '\b') {
                    if (ptr > 0) {
                        ptr--;
                        if (echo) {
                            terminal_putchar('\b');
                            terminal_putchar(' ');
                            terminal_putchar('\b');
                        }
                    }
                } else if (ptr < max && c != 0) {
                    buffer[ptr++] = c;
                    if (echo) terminal_putchar(c);
                }
            }
        }
    }
}

static char shell_buf[256];
static int shell_ptr = 0;

// Эту функцию вызывает драйвер клавиатуры
void shell_handle_key(char c) {
    if (c == '\n') {
        terminal_putchar('\n');
        shell_buf[shell_ptr] = '\0';
        
        if (shell_ptr > 0) {
            kernel_execute_command(shell_buf);
        }

        terminal_writestring("LakOS>");
        if (strcmp(current_user, "root") == 0) {
            terminal_writestring("\033[31mroot\033[0m");
        } else {
            terminal_writestring("\033[32m");
            terminal_writestring(current_user);
            terminal_writestring("\033[0m");
        }
        terminal_writestring(" ");
        terminal_writestring(current_dir);
        terminal_writestring(" \033[36m(uid:");
        char buf[16];
        itoa(get_current_uid(), buf);
        terminal_writestring(buf);
        terminal_writestring(",gid:");
        itoa(get_current_gid(), buf);
        terminal_writestring(buf);
        terminal_writestring(")\033[0m ");
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
    terminal_writestring("Shell start\n");
    terminal_writestring("lkaS  ");
    terminal_writestring(SHELL_VERSION);
    terminal_writestring("\n");
    init_users();
    terminal_writestring("Users done\n");
    terminal_writestring("\n");
    terminal_writestring(" _         _    ___  ___ \n");
    terminal_writestring("| |   ___ | |__| . |/ __>  by @lakladn\n");
    terminal_writestring("| |_ <_> || / /| | |\\__ \\\n");
    terminal_writestring("|___|<___||_\\_\\`___'<___/  ONO SUKA RABOTAET\n");
    terminal_writestring("\nWelcome to Lakos OS\n");

    terminal_writestring("Shell initialized, starting login\n");

    // Debug: Check current_user before login
    terminal_writestring("DEBUG: current_user before login = ");
    terminal_writestring(current_user);
    terminal_writestring("\n");

    // Login
    char username[32];
    char password[32];
    while (1) {
        terminal_writestring("Login: ");
        read_line(username, 31, 1);
        terminal_writestring("Password: ");
        read_line(password, 31, 0);
        terminal_putchar('\n');
        
        // Debug output for troubleshooting
        terminal_writestring("Attempting login for user: ");
        terminal_writestring(username);
        terminal_writestring("\n");
        
        if (authenticate_user(username, password)) {
            terminal_writestring("Login successful for user: ");
            terminal_writestring(current_user);
            terminal_writestring("\n");
            break;
        } else {
            terminal_writestring("Invalid login\n");
        }
    }

    terminal_writestring("Login successful\n");
    terminal_writestring("LakOS>");
    if (strcmp(current_user, "root") == 0) {
        terminal_writestring("\033[31mroot\033[0m");
    } else {
        terminal_writestring("\033[32m");
        terminal_writestring(current_user);
        terminal_writestring("\033[0m");
    }
    terminal_writestring(" / \033[36m(uid:");
    char buf[16];
    itoa(get_current_uid(), buf);
    terminal_writestring(buf);
    terminal_writestring(",gid:");
    itoa(get_current_gid(), buf);
    terminal_writestring(buf);
    terminal_writestring(")\033[0m ");
    while(1) {
        if (inb(0x64) & 0x1) {
            uint8_t scancode = inb(0x60);
            if ((scancode & 0x7F) == 42 || (scancode & 0x7F) == 54) {
                shift_pressed = !(scancode & 0x80);
            } else if (scancode == 58) {
                if (!(scancode & 0x80)) caps_locked = !caps_locked;
            } else if (!(scancode & 0x80)) {
                int is_letter = (scancode >= 16 && scancode <= 25) || (scancode >= 30 && scancode <= 38) || (scancode >= 44 && scancode <= 50);
                int uppercase = shift_pressed || (caps_locked && is_letter);
                char c = uppercase ? kbd_map_shift[scancode] : kbd_map[scancode];
                if (c != 0) shell_handle_key(c);
            }
        }
    }
}