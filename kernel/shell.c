/*
 * Lakos OS
 * Copyright (c) 2026 lakladon
 * Created: January 8, 2026
 */

#include <stdint.h>
#include <io.h>
#include "include/users.h"
#include "include/version.h"
#include "include/lib.h"
#include "include/commands.h"

extern void terminal_writestring(const char* s);
extern void terminal_putchar(char c);
extern void terminal_initialize();
extern void terminal_display_time();

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

// Russian keyboard layout (ЙЦУКЕНГШЩЗХЪ)
// Using extended ASCII codes: 128-191 for Cyrillic (see font.c)
// А=128, Б=129, В=130, Г=131, Д=132, Е=133, Ё=134, Ж=135, З=136, И=137, Й=138
// К=139, Л=140, М=141, Н=142, О=143, П=144, Р=145, С=146, Т=147, У=148, Ф=149
// Х=150, Ц=151, Ч=152, Ш=153, Щ=154, Ъ=155, Ы=156, Ь=157, Э=158, Ю=159, Я=160
// а=161, б=162, в=163, г=164, д=165, е=166, ё=167, ж=168, з=169, и=170, й=171
// к=172, л=173, м=174, н=175, о=176, п=177, р=178, с=179, т=180, у=181, ф=182
// х=183, ц=184, ч=185, ш=186, щ=187, ъ=188, ы=189, ь=190, э=191, ю=192, я=193

// Russian keyboard layout (lowercase) - standard ЙЦУКЕН layout
// Scancodes: Q=16, W=17, E=18, R=19, T=20, Y=21, U=22, I=23, O=24, P=25, [=26, ]=27
//            A=30, S=31, D=32, F=33, G=34, H=35, J=36, K=37, L=38, ;=39, '=40
//            Z=44, X=45, C=46, V=47, B=48, N=49, M=50, ,=51, .=52
// Font codes from font.c:
//   а=160, б=161, в=162, г=163, д=164, е=165, ё=166, ж=167, з=168, и=169, й=170
//   к=171, л=172, м=173, н=174, о=175, п=176, р=177, с=178, т=179, у=180, ф=181
//   х=182, ц=183, ч=184, ш=185, щ=186, ъ=187, ы=188, ь=189, э=190, ю=191
unsigned char kbd_map_ru[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 170, 183, 180, 171, 165, 174, 163, 185, 186, 168, 182, 187, 0, '\n',  // йцукенгшщзхъ
    0, 181, 188, 162, 160, 176, 177, 175, 172, 164, 167, 190, 0,               // фывапролджэ
    '\\', 191, 184, 178, 173, 169, 179, 189, 161, 191, 0, '*', 0, ' ', 0       // ячсмитьбю (я=191 temporarily)
};

// Russian keyboard layout (uppercase) - with Shift
// Font codes from font.c:
//   А=128, Б=129, В=130, Г=131, Д=132, Е=133, Ё=134, Ж=135, З=136, И=137, Й=138
//   К=139, Л=140, М=141, Н=142, О=143, П=144, Р=145, С=146, Т=147, У=148, Ф=149
//   Х=150, Ц=151, Ч=152, Ш=153, Щ=154, Ъ=155, Ы=156, Ь=157, Э=158, Ю=159
unsigned char kbd_map_ru_shift[128] = {
    0,  27, '!', '"', '#', ';', '%', ':', '?', '*', '(', ')', '_', '+', '\b',
    '\t', 138, 151, 148, 139, 133, 142, 131, 153, 154, 136, 150, 155, 0, '\n',  // ЙЦУКЕНГШЩЗХЪ
    0, 149, 156, 130, 128, 144, 145, 143, 140, 132, 135, 158, 0,               // ФЫВАПРОЛДЖЭ
    '/', 159, 152, 146, 141, 137, 147, 157, 129, 159, 0, '*', 0, ' ', 0        // ЯЧСМИТЬБЮ (Я=159 temporarily)
};

static int shift_pressed = 0;
static int caps_locked = 0;
static int alt_pressed = 0;
static int current_layout = 0; // 0 = English, 1 = Russian

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
static int cursor_pos = 0;  // Current cursor position in the buffer

// Command history
#define HISTORY_SIZE 10
static char command_history[HISTORY_SIZE][256];
static int history_count = 0;
static int history_index = 0;

// Command completion
static const char* available_commands[] = {
    "help", "man", "cls", "ver", "pwd", "ls", "cd", "echo", "uname", "date", 
    "cat", "mkdir", "disks", "read_sector", "write_sector", "mount",
    "useradd", "passwd", "login", "userdel", "crypt", "whoami", 
    "touch", "rm", "cp", "shutdown", "reboot", "gui", "hello", "test", "editor", "calc", "asm", "colorb", "lsh"
};
static int commands_count = 36;

// Arrow key scancodes
#define KEY_UP 72
#define KEY_DOWN 80
#define KEY_LEFT 75
#define KEY_RIGHT 77
#define KEY_TAB 15

// External functions for cursor control
extern void terminal_move_cursor(int col);
extern int terminal_get_cursor_col();
extern int terminal_get_cursor_row();
extern void terminal_putchar_at(int col, int row, char c);
extern char terminal_getchar_at(int col, int row);
extern void terminal_update_cursor();

// Prompt length (used for cursor positioning)
static int prompt_length = 0;

// Function to redraw the current line
void redraw_line() {
    // Get current row
    int row = terminal_get_cursor_row();
    int start_col = prompt_length;
    
    // Clear the line from start_col to end using direct video memory write
    // This avoids triggering scroll
    for (int i = start_col; i < 80; i++) {
        terminal_putchar_at(i, row, ' ');
    }
    
    // Redraw the buffer content
    for (int i = 0; i < shell_ptr; i++) {
        terminal_putchar_at(start_col + i, row, shell_buf[i]);
    }
    
    // Position cursor at cursor_pos
    terminal_move_cursor(start_col + cursor_pos);
}

// Function to move cursor left
void move_cursor_left() {
    if (cursor_pos > 0) {
        cursor_pos--;
        terminal_move_cursor(prompt_length + cursor_pos);
    }
}

// Function to move cursor right
void move_cursor_right() {
    if (cursor_pos < shell_ptr) {
        cursor_pos++;
        terminal_move_cursor(prompt_length + cursor_pos);
    }
}

// Function to insert character at cursor position
void insert_char(char c) {
    if (shell_ptr < 255) {
        // Shift characters to the right
        for (int i = shell_ptr; i > cursor_pos; i--) {
            shell_buf[i] = shell_buf[i - 1];
        }
        shell_buf[cursor_pos] = c;
        shell_ptr++;
        cursor_pos++;
        
        // Redraw from cursor position
        redraw_line();
    }
}

// Function to delete character at cursor position (backspace)
void delete_char_before() {
    if (cursor_pos > 0) {
        // Shift characters to the left
        for (int i = cursor_pos - 1; i < shell_ptr - 1; i++) {
            shell_buf[i] = shell_buf[i + 1];
        }
        shell_ptr--;
        cursor_pos--;
        shell_buf[shell_ptr] = '\0';
        
        // Redraw line
        redraw_line();
    }
}

// Function to delete character at cursor position (delete key)
void delete_char_at() {
    if (cursor_pos < shell_ptr) {
        // Shift characters to the left
        for (int i = cursor_pos; i < shell_ptr - 1; i++) {
            shell_buf[i] = shell_buf[i + 1];
        }
        shell_ptr--;
        shell_buf[shell_ptr] = '\0';
        
        // Redraw line
        redraw_line();
    }
}

// Function to add command to history
void add_to_history(const char* command) {
    if (strlen(command) > 0) {
        // Add to history
        strcpy(command_history[history_count % HISTORY_SIZE], command);
        history_count++;
        history_index = history_count; // Point to newest command
    }
}

// Function to get previous command from history
void get_previous_history() {
    if (history_count > 0 && history_index > 0) {
        history_index--;
        int prev_index = history_index % HISTORY_SIZE;
        
        // Load previous command
        strcpy(shell_buf, command_history[prev_index]);
        shell_ptr = strlen(shell_buf);
        cursor_pos = shell_ptr;  // Move cursor to end
        
        // Redraw line
        redraw_line();
    }
}

// Function to get next command from history
void get_next_history() {
    if (history_count > 0) {
        if (history_index < history_count - 1) {
            history_index++;
            int next_index = history_index % HISTORY_SIZE;
            
            // Load next command
            strcpy(shell_buf, command_history[next_index]);
            shell_ptr = strlen(shell_buf);
            cursor_pos = shell_ptr;  // Move cursor to end
            
            // Redraw line
            redraw_line();
        } else if (history_index == history_count - 1) {
            // At the newest entry, go to empty line
            history_index++;
            
            shell_ptr = 0;
            cursor_pos = 0;
            shell_buf[0] = '\0';
            
            // Redraw line (clear it)
            redraw_line();
        }
    }
}

// Function to find matching commands for tab completion
int find_command_matches(const char* prefix, char matches[][32]) {
    int match_count = 0;
    int prefix_len = strlen(prefix);
    
    for (int i = 0; i < commands_count; i++) {
        if (strncmp(available_commands[i], prefix, prefix_len) == 0) {
            strcpy(matches[match_count], available_commands[i]);
            match_count++;
        }
    }
    return match_count;
}

// Function to find directory matches for cd command
int find_directory_matches(const char* prefix, char matches[][32]) {
    int match_count = 0;
    int prefix_len = strlen(prefix);
    
    // For now, provide basic directory completion without accessing commands.c variables
    // This is a simplified version that doesn't require external variable access
    if (strcmp(prefix, "") == 0) {
        // Show basic directories when no prefix
        strcpy(matches[match_count], "bin/");
        match_count++;
        strcpy(matches[match_count], "home/");
        match_count++;
    } else if (strncmp(prefix, "bin", prefix_len) == 0) {
        strcpy(matches[match_count], "bin/");
        match_count++;
    } else if (strncmp(prefix, "home", prefix_len) == 0) {
        strcpy(matches[match_count], "home/");
        match_count++;
    }
    return match_count;
}

// Function to perform tab completion
void perform_completion() {
    if (shell_ptr == 0) return;
    
    // Find the current word being typed
    int start = 0;
    for (int i = shell_ptr - 1; i >= 0; i--) {
        if (shell_buf[i] == ' ') {
            start = i + 1;
            break;
        }
    }
    
    char current_word[32];
    int word_len = shell_ptr - start;
    if (word_len >= 32) word_len = 31;
    for (int i = 0; i < word_len; i++) {
        current_word[i] = shell_buf[start + i];
    }
    current_word[word_len] = '\0';
    
    // Check if this is a cd command
    int is_cd_command = 0;
    if (start == 0) {
        // Check if first word is "cd"
        char first_word[32];
        int space_pos = -1;
        for (int i = 0; i < shell_ptr; i++) {
            if (shell_buf[i] == ' ') {
                space_pos = i;
                break;
            }
        }
        if (space_pos > 0) {
            int len = space_pos;
            if (len >= 32) len = 31;
            for (int i = 0; i < len; i++) {
                first_word[i] = shell_buf[i];
            }
            first_word[len] = '\0';
            if (strcmp(first_word, "cd") == 0) {
                is_cd_command = 1;
            }
        }
    }
    
    char matches[10][32];
    int match_count = 0;
    
    if (is_cd_command) {
        match_count = find_directory_matches(current_word, matches);
    } else {
        match_count = find_command_matches(current_word, matches);
    }
    
    if (match_count == 1) {
        // Single match - complete it
        // Remove current word
        for (int i = 0; i < word_len; i++) {
            terminal_putchar('\b');
            terminal_putchar(' ');
            terminal_putchar('\b');
        }
        // Add the completed word
        strcpy(shell_buf + start, matches[0]);
        shell_ptr = start + strlen(matches[0]);
        terminal_writestring(matches[0]);
    } else if (match_count > 1) {
        // Multiple matches - show them and keep current input
        terminal_putchar('\n');
        for (int i = 0; i < match_count; i++) {
            terminal_writestring(matches[i]);
            terminal_writestring(" ");
        }
        terminal_putchar('\n');
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
        terminal_writestring(shell_buf);
    }
}

// Calculate prompt length for cursor positioning
int calculate_prompt_length() {
    int len = 6; // "LakOS>"
    len += strlen(current_user);
    len += 1; // space
    len += strlen(current_dir);
    len += 12; // " (uid:X,gid:Y)" approximately
    // Count digits in uid and gid
    int uid = get_current_uid();
    int gid = get_current_gid();
    if (uid == 0) len += 1;
    else while (uid > 0) { len++; uid /= 10; }
    if (gid == 0) len += 1;
    else while (gid > 0) { len++; gid /= 10; }
    len += 1; // final space
    return len;
}

// Display the shell prompt
void display_prompt() {
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
    prompt_length = calculate_prompt_length();
}

// Эту функцию вызывает драйвер клавиатуры
void shell_handle_key(char c) {
    if (c == '\n') {
        terminal_putchar('\n');
        shell_buf[shell_ptr] = '\0';
        
        if (shell_ptr > 0) {
            // Add command to history before executing
            add_to_history(shell_buf);
            kernel_execute_command(shell_buf);
        }

        // Reset buffer and cursor
        shell_ptr = 0;
        cursor_pos = 0;
        
        display_prompt();
    } 
    else if (c == '\b') {
        delete_char_before();
    } 
    else if (shell_ptr < 255) {
        insert_char(c);
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
        // Trim whitespace from username before authentication
        char trimmed_username[32];
        int src = 0, dst = 0;
        while (username[src] == ' ' || username[src] == '\t') src++; // skip leading whitespace
        while (username[src] != '\0') {
            if (username[src] != ' ' && username[src] != '\t') {
                trimmed_username[dst++] = username[src];
            }
            src++;
        }
        trimmed_username[dst] = '\0';
        
        terminal_writestring("Attempting login for user: ");
        terminal_writestring(trimmed_username);
        terminal_writestring("\n");
        
        if (authenticate_user(trimmed_username, password)) {
            terminal_writestring("Login successful for user: ");
            terminal_writestring(current_user);
            terminal_writestring("\n");
            break;
        } else {
            terminal_writestring("Invalid login\n");
        }
    }

    terminal_writestring("Login successful\n");
    
    // Display initial prompt and set prompt_length
    display_prompt();
    
    // Display initial time
    terminal_display_time();
    
    while(1) {
        // Update time display on every iteration
        terminal_display_time();
        
        if (inb(0x64) & 0x1) {
            uint8_t scancode = inb(0x60);
            
            // Track Alt key (scancode 56 = Left Alt, 0x38 = release with 0x80)
            if ((scancode & 0x7F) == 56) {
                alt_pressed = !(scancode & 0x80);
            }
            
            // Track Shift keys (scancode 42 = Left Shift, 54 = Right Shift)
            if ((scancode & 0x7F) == 42 || (scancode & 0x7F) == 54) {
                // Check for Alt+Shift combination to switch layout
                if (!(scancode & 0x80) && alt_pressed) {
                    // Alt+Shift pressed - toggle layout
                    current_layout = !current_layout;
                    // Display layout indicator
                    terminal_writestring("\n[Layout: ");
                    terminal_writestring(current_layout ? "RU" : "EN");
                    terminal_writestring("]\n");
                    display_prompt();
                    if (shell_ptr > 0) {
                        terminal_writestring(shell_buf);
                    }
                }
                shift_pressed = !(scancode & 0x80);
            } else if (scancode == 58) {
                if (!(scancode & 0x80)) caps_locked = !caps_locked;
            } else if (!(scancode & 0x80)) {
                if (scancode == KEY_UP) {
                    get_previous_history();
                } else if (scancode == KEY_DOWN) {
                    get_next_history();
                } else if (scancode == KEY_LEFT) {
                    move_cursor_left();
                } else if (scancode == KEY_RIGHT) {
                    move_cursor_right();
                } else if (scancode == KEY_TAB) {
                    perform_completion();
                } else {
                    int is_letter = (scancode >= 16 && scancode <= 25) || (scancode >= 30 && scancode <= 38) || (scancode >= 44 && scancode <= 50);
                    int uppercase = shift_pressed || (caps_locked && is_letter);
                    
                    char c;
                    if (current_layout == 0) {
                        // English layout
                        c = uppercase ? kbd_map_shift[scancode] : kbd_map[scancode];
                    } else {
                        // Russian layout
                        c = uppercase ? kbd_map_ru_shift[scancode] : kbd_map_ru[scancode];
                    }
                    
                    if (c != 0) shell_handle_key(c);
                }
            }
        }
    }
}
