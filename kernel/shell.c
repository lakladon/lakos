extern void terminal_writestring(const char* s)
extern void terminal_putchar(char c)
extern void terminal_initialize()
extern void terminal_display_time()
unsigned char kbd_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0
}
unsigned char kbd_map_shift[128] = {
    0,  27, '!', '@', '
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0
}
static int shift_pressed = 0
static int caps_locked = 0
void read_line(char* buffer, int max, int echo) {
    int ptr = 0
    while (1) {
        if (inb(0x64) & 0x1) {
            uint8_t scancode = inb(0x60)
            if ((scancode & 0x7F) == 42 || (scancode & 0x7F) == 54) {
                shift_pressed = !(scancode & 0x80)
            } else if (scancode == 58) {
                if (!(scancode & 0x80)) caps_locked = !caps_locked
            } else if (!(scancode & 0x80)) {
                int is_letter = (scancode >= 16 && scancode <= 25) || (scancode >= 30 && scancode <= 38) || (scancode >= 44 && scancode <= 50)
                int uppercase = shift_pressed || (caps_locked && is_letter)
                char c = uppercase ? kbd_map_shift[scancode] : kbd_map[scancode]
                if (c == '\n') {
                    buffer[ptr] = '\0'
                    if (echo) terminal_putchar('\n')
                    return
                } else if (c == '\b') {
                    if (ptr > 0) {
                        ptr--
                        if (echo) {
                            terminal_putchar('\b')
                            terminal_putchar(' ')
                            terminal_putchar('\b')
                        }
                    }
                } else if (ptr < max && c != 0) {
                    buffer[ptr++] = c
                    if (echo) terminal_putchar(c)
                }
            }
        }
    }
}
static char shell_buf[256]
static int shell_ptr = 0
static int cursor_pos = 0
static char command_history[HISTORY_SIZE][256]
static int history_count = 0
static int history_index = 0
static const char* available_commands[] = {
    "help", "man", "cls", "ver", "pwd", "ls", "cd", "echo", "uname", "date", 
    "cat", "mkdir", "disks", "read_sector", "write_sector", "mount",
    "useradd", "passwd", "login", "userdel", "crypt", "whoami", 
    "touch", "rm", "cp", "shutdown", "reboot", "gui", "hello", "test", "editor", "calc", "asm", "colorb", "lsh"
}
static int commands_count = 36
extern void terminal_move_cursor(int col)
extern int terminal_get_cursor_col()
extern int terminal_get_cursor_row()
extern void terminal_putchar_at(int col, int row, char c)
extern char terminal_getchar_at(int col, int row)
extern void terminal_update_cursor()
static int prompt_length = 0
void redraw_line() {
    int row = terminal_get_cursor_row()
    int start_col = prompt_length
    for (int i = start_col
        terminal_putchar_at(i, row, ' ')
    }
    for (int i = 0
        terminal_putchar_at(start_col + i, row, shell_buf[i])
    }
    terminal_move_cursor(start_col + cursor_pos)
}
void move_cursor_left() {
    if (cursor_pos > 0) {
        cursor_pos--
        terminal_move_cursor(prompt_length + cursor_pos)
    }
}
void move_cursor_right() {
    if (cursor_pos < shell_ptr) {
        cursor_pos++
        terminal_move_cursor(prompt_length + cursor_pos)
    }
}
void insert_char(char c) {
    if (shell_ptr < 255) {
        for (int i = shell_ptr
            shell_buf[i] = shell_buf[i - 1]
        }
        shell_buf[cursor_pos] = c
        shell_ptr++
        cursor_pos++
        redraw_line()
    }
}
void delete_char_before() {
    if (cursor_pos > 0) {
        for (int i = cursor_pos - 1
            shell_buf[i] = shell_buf[i + 1]
        }
        shell_ptr--
        cursor_pos--
        shell_buf[shell_ptr] = '\0'
        redraw_line()
    }
}
void delete_char_at() {
    if (cursor_pos < shell_ptr) {
        for (int i = cursor_pos
            shell_buf[i] = shell_buf[i + 1]
        }
        shell_ptr--
        shell_buf[shell_ptr] = '\0'
        redraw_line()
    }
}
void add_to_history(const char* command) {
    if (strlen(command) > 0) {
        strcpy(command_history[history_count % HISTORY_SIZE], command)
        history_count++
        history_index = history_count
    }
}
void get_previous_history() {
    if (history_count > 0 && history_index > 0) {
        history_index--
        int prev_index = history_index % HISTORY_SIZE
        strcpy(shell_buf, command_history[prev_index])
        shell_ptr = strlen(shell_buf)
        cursor_pos = shell_ptr
        redraw_line()
    }
}
void get_next_history() {
    if (history_count > 0) {
        if (history_index < history_count - 1) {
            history_index++
            int next_index = history_index % HISTORY_SIZE
            strcpy(shell_buf, command_history[next_index])
            shell_ptr = strlen(shell_buf)
            cursor_pos = shell_ptr
            redraw_line()
        } else if (history_index == history_count - 1) {
            history_index++
            shell_ptr = 0
            cursor_pos = 0
            shell_buf[0] = '\0'
            redraw_line()
        }
    }
}
int find_command_matches(const char* prefix, char matches[][32]) {
    int match_count = 0
    int prefix_len = strlen(prefix)
    for (int i = 0
        if (strncmp(available_commands[i], prefix, prefix_len) == 0) {
            strcpy(matches[match_count], available_commands[i])
            match_count++
        }
    }
    return match_count
}
int find_directory_matches(const char* prefix, char matches[][32]) {
    int match_count = 0
    int prefix_len = strlen(prefix)
    if (strcmp(prefix, "") == 0) {
        strcpy(matches[match_count], "bin/")
        match_count++
        strcpy(matches[match_count], "home/")
        match_count++
    } else if (strncmp(prefix, "bin", prefix_len) == 0) {
        strcpy(matches[match_count], "bin/")
        match_count++
    } else if (strncmp(prefix, "home", prefix_len) == 0) {
        strcpy(matches[match_count], "home/")
        match_count++
    }
    return match_count
}
void perform_completion() {
    if (shell_ptr == 0) return
    int start = 0
    for (int i = shell_ptr - 1
        if (shell_buf[i] == ' ') {
            start = i + 1
            break
        }
    }
    char current_word[32]
    int word_len = shell_ptr - start
    if (word_len >= 32) word_len = 31
    for (int i = 0
        current_word[i] = shell_buf[start + i]
    }
    current_word[word_len] = '\0'
    int is_cd_command = 0
    if (start == 0) {
        char first_word[32]
        int space_pos = -1
        for (int i = 0
            if (shell_buf[i] == ' ') {
                space_pos = i
                break
            }
        }
        if (space_pos > 0) {
            int len = space_pos
            if (len >= 32) len = 31
            for (int i = 0
                first_word[i] = shell_buf[i]
            }
            first_word[len] = '\0'
            if (strcmp(first_word, "cd") == 0) {
                is_cd_command = 1
            }
        }
    }
    char matches[10][32]
    int match_count = 0
    if (is_cd_command) {
        match_count = find_directory_matches(current_word, matches)
    } else {
        match_count = find_command_matches(current_word, matches)
    }
    if (match_count == 1) {
        for (int i = 0
            terminal_putchar('\b')
            terminal_putchar(' ')
            terminal_putchar('\b')
        }
        strcpy(shell_buf + start, matches[0])
        shell_ptr = start + strlen(matches[0])
        terminal_writestring(matches[0])
    } else if (match_count > 1) {
        terminal_putchar('\n')
        for (int i = 0
            terminal_writestring(matches[i])
            terminal_writestring(" ")
        }
        terminal_putchar('\n')
        terminal_writestring("LakOS>")
        if (strcmp(current_user, "root") == 0) {
            terminal_writestring("\033[31mroot\033[0m")
        } else {
            terminal_writestring("\033[32m")
            terminal_writestring(current_user)
            terminal_writestring("\033[0m")
        }
        terminal_writestring(" ")
        terminal_writestring(current_dir)
        terminal_writestring(" \033[36m(uid:")
        char buf[16]
        itoa(get_current_uid(), buf)
        terminal_writestring(buf)
        terminal_writestring(",gid:")
        itoa(get_current_gid(), buf)
        terminal_writestring(buf)
        terminal_writestring(")\033[0m ")
        terminal_writestring(shell_buf)
    }
}
int calculate_prompt_length() {
    int len = 6
    len += strlen(current_user)
    len += 1
    len += strlen(current_dir)
    len += 12
    int uid = get_current_uid()
    int gid = get_current_gid()
    if (uid == 0) len += 1
    else while (uid > 0) { len++
    if (gid == 0) len += 1
    else while (gid > 0) { len++
    len += 1
    return len
}
void display_prompt() {
    terminal_writestring("LakOS>")
    if (strcmp(current_user, "root") == 0) {
        terminal_writestring("\033[31mroot\033[0m")
    } else {
        terminal_writestring("\033[32m")
        terminal_writestring(current_user)
        terminal_writestring("\033[0m")
    }
    terminal_writestring(" ")
    terminal_writestring(current_dir)
    terminal_writestring(" \033[36m(uid:")
    char buf[16]
    itoa(get_current_uid(), buf)
    terminal_writestring(buf)
    terminal_writestring(",gid:")
    itoa(get_current_gid(), buf)
    terminal_writestring(buf)
    terminal_writestring(")\033[0m ")
    prompt_length = calculate_prompt_length()
}
void shell_handle_key(char c) {
    if (c == '\n') {
        terminal_putchar('\n')
        shell_buf[shell_ptr] = '\0'
        if (shell_ptr > 0) {
            add_to_history(shell_buf)
            kernel_execute_command(shell_buf)
        }
        shell_ptr = 0
        cursor_pos = 0
        display_prompt()
    } 
    else if (c == '\b') {
        delete_char_before()
    } 
    else if (shell_ptr < 255) {
        insert_char(c)
    }
}
void shell_main() {
    terminal_writestring("Shell start\n")
    terminal_writestring("lkaS  ")
    terminal_writestring(SHELL_VERSION)
    terminal_writestring("\n")
    init_users()
    terminal_writestring("Users done\n")
    terminal_writestring("\n")
    terminal_writestring(" _         _    ___  ___ \n")
    terminal_writestring("| |   ___ | |__| . |/ __>  by @lakladn\n")
    terminal_writestring("| |_ <_> || / /| | |\\__ \\\n")
    terminal_writestring("|___|<___||_\\_\\`___'<___/  ONO SUKA RABOTAET\n")
    terminal_writestring("\nWelcome to Lakos OS\n")
    char username[32]
    char password[32]
    while (1) {
        terminal_writestring("Login: ")
        read_line(username, 31, 1)
        terminal_writestring("Password: ")
        read_line(password, 31, 0)
        terminal_putchar('\n')
        char trimmed_username[32]
        int src = 0, dst = 0
        while (username[src] == ' ' || username[src] == '\t') src++
        while (username[src] != '\0') {
            if (username[src] != ' ' && username[src] != '\t') {
                trimmed_username[dst++] = username[src]
            }
            src++
        }
        trimmed_username[dst] = '\0'
        terminal_writestring("Attempting login for user: ")
        terminal_writestring(trimmed_username)
        terminal_writestring("\n")
        if (authenticate_user(trimmed_username, password)) {
            terminal_writestring("Login successful for user: ")
            terminal_writestring(current_user)
            terminal_writestring("\n")
            break
        } else {
            terminal_writestring("Invalid login\n")
        }
    }
    terminal_writestring("Login successful\n")
    display_prompt()
    terminal_display_time()
    while(1) {
        terminal_display_time()
        if (inb(0x64) & 0x1) {
            uint8_t scancode = inb(0x60)
            if ((scancode & 0x7F) == 42 || (scancode & 0x7F) == 54) {
                shift_pressed = !(scancode & 0x80)
            } else if (scancode == 58) {
                if (!(scancode & 0x80)) caps_locked = !caps_locked
            } else if (!(scancode & 0x80)) {
                if (scancode == KEY_UP) {
                    get_previous_history()
                } else if (scancode == KEY_DOWN) {
                    get_next_history()
                } else if (scancode == KEY_LEFT) {
                    move_cursor_left()
                } else if (scancode == KEY_RIGHT) {
                    move_cursor_right()
                } else if (scancode == KEY_TAB) {
                    perform_completion()
                } else {
                    int is_letter = (scancode >= 16 && scancode <= 25) || (scancode >= 30 && scancode <= 38) || (scancode >= 44 && scancode <= 50)
                    int uppercase = shift_pressed || (caps_locked && is_letter)
                    char c = uppercase ? kbd_map_shift[scancode] : kbd_map[scancode]
                    if (c != 0) shell_handle_key(c)
                }
            }
        }
    }
}
