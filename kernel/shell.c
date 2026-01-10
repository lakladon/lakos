#include <stdint.h>
#include <io.h>

void strcpy(char* dest, const char* src) {
    while (*src) *dest++ = *src++;
    *dest = '\0';
}

int strncmp(const char* s1, const char* s2, unsigned int n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++; s2++; n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

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
static char current_dir[256] = "/";
static char* pathbin = "/bin";

// Simple file system simulation
typedef struct {
    char name[32];
    char content[256];
    int size;
} file_t;

file_t files[10];
int file_count = 0;

char dirs[10][32];
int dir_count = 3; // bin, dev, home

char home_dirs[10][32];
int home_dir_count = 0;
char home_subdirs[10][10][32];
int home_sub_count[10] = {0};

void init_dirs() {
    strcpy(dirs[0], "bin");
    strcpy(dirs[1], "dev");
    strcpy(dirs[2], "home");
}

static char shell_buf[256];
static int shell_ptr = 0;

// Функция сравнения строк (без нее команды не найти)
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strlen(const char* s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

char* strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        const char* h = haystack;
        const char* n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return (char*)haystack;
    }
    return 0;
}

char* strncat(char* dest, const char* src, int n) {
    char* d = dest;
    while (*d) d++;
    while (n-- && *src) *d++ = *src++;
    *d = '\0';
    return dest;
}

int is_file_in_path(const char* name, const char* path) {
    if (strcmp(path, "/bin") == 0 && (strcmp(name, "hello") == 0 || strcmp(name, "test") == 0 || strcmp(name, "editor") == 0)) return 1;
    return 0;
}

file_t* find_file(const char* name) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, name) == 0) return &files[i];
    }
    return 0;
}

file_t* create_file(const char* name) {
    if (file_count >= 10) return 0;
    int len = strlen(name);
    if (len >= 32) return 0;
    strcpy(files[file_count].name, name);
    files[file_count].content[0] = '\0';
    files[file_count].size = 0;
    return &files[file_count++];
}

// Твоя функция поиска команд и бинарников
void writeUSERterminal(const char* input) {
    if (strcmp(input, "help") == 0) {
        terminal_writestring("Lakos OS Commands: help, cls, ver, pwd, ls, cd, echo, uname, date, cat, mkdir, disks, mount\nAvailable programs: hello, test, editor\n");
    }
    else if (strcmp(input, "cls") == 0) {
        terminal_initialize();
    }
    else if (strcmp(input, "ver") == 0) {
        terminal_writestring("Lakos OS v0.3.7 [Kernel Mode]\n");
    }
    else if (strcmp(input, "pwd") == 0) {
        terminal_writestring(current_dir);
        terminal_writestring("\n");
    }
    else if (strcmp(input, "ls") == 0) {
        if (strcmp(current_dir, "/") == 0) {
            for (int i = 0; i < dir_count; i++) {
                terminal_writestring(dirs[i]);
                terminal_writestring("/ ");
            }
            terminal_writestring("\n");
        } else if (strcmp(current_dir, "/bin") == 0) {
            terminal_writestring("hello  test  editor\n");
        } else if (strcmp(current_dir, "/home") == 0) {
            for (int i = 0; i < home_dir_count; i++) {
                terminal_writestring(home_dirs[i]);
                terminal_writestring("/ ");
            }
            terminal_writestring("\n");
        } else if (strncmp(current_dir, "/home/", 6) == 0) {
            const char* dir = current_dir + 6;
            int dir_len = 0;
            while (dir[dir_len] && dir[dir_len] != '/') dir_len++;
            char dir_name[32];
            if (dir_len >= 32) dir_len = 31;
            for (int k = 0; k < dir_len; k++) dir_name[k] = dir[k];
            dir_name[dir_len] = '\0';
            for (int i = 0; i < home_dir_count; i++) {
                if (strcmp(home_dirs[i], dir_name) == 0) {
                    for (int j = 0; j < home_sub_count[i]; j++) {
                        terminal_writestring(home_subdirs[i][j]);
                        terminal_writestring("/ ");
                    }
                    terminal_writestring("\n");
                    return;
                }
            }
            terminal_writestring(".\n");
        } else {
            terminal_writestring(".\n");
        }
    }
    else if (strncmp(input, "cd ", 3) == 0) {
        const char* dir = input + 3;
        if (strlen(dir) == 0) {
            strcpy(current_dir, "/home");
        } else if (strcmp(dir, "/") == 0) {
            strcpy(current_dir, "/");
        } else if (strcmp(dir, "bin") == 0) {
            strcpy(current_dir, "/bin");
        } else if (strcmp(dir, "home") == 0) {
            strcpy(current_dir, "/home");
        } else if (strcmp(dir, "..") == 0) {
            if (strcmp(current_dir, "/") == 0) {
                // stay
            } else if (strcmp(current_dir, "/home") == 0 || strcmp(current_dir, "/bin") == 0) {
                strcpy(current_dir, "/");
            } else if (strncmp(current_dir, "/home/", 6) == 0) {
                char* last_slash = current_dir + strlen(current_dir);
                while (last_slash > current_dir && *last_slash != '/') last_slash--;
                if (last_slash > current_dir + 5) {
                    *last_slash = '\0';
                } else {
                    strcpy(current_dir, "/home");
                }
            } else {
                strcpy(current_dir, "/");
            }
        } else if (strcmp(current_dir, "/home") == 0) {
            strcpy(current_dir, "/home/");
            int remaining = 255 - strlen(current_dir);
            int dir_len = strlen(dir);
            if (dir_len > remaining) {
                terminal_writestring("cd: directory name too long\n");
            } else {
                strncat(current_dir, dir, remaining);
            }
        } else if (strncmp(current_dir, "/home/", 6) == 0) {
            const char* parent = current_dir + 6;
            int parent_len = 0;
            while (parent[parent_len] && parent[parent_len] != '/') parent_len++;
            char parent_name[32];
            if (parent_len >= 32) parent_len = 31;
            for (int k = 0; k < parent_len; k++) parent_name[k] = parent[k];
            parent_name[parent_len] = '\0';
            int found = 0;
            for (int i = 0; i < home_dir_count; i++) {
                if (strcmp(home_dirs[i], parent_name) == 0) {
                    for (int j = 0; j < home_sub_count[i]; j++) {
                        if (strcmp(home_subdirs[i][j], dir) == 0) {
                            strncat(current_dir, "/", 255 - strlen(current_dir));
                            strncat(current_dir, dir, 255 - strlen(current_dir));
                            found = 1;
                            break;
                        }
                    }
                    break;
                }
            }
            if (!found) {
                terminal_writestring("cd: ");
                terminal_writestring(dir);
                terminal_writestring(": No such file or directory\n");
            }
        } else {
            terminal_writestring("cd: ");
            terminal_writestring(dir);
            terminal_writestring(": No such file or directory\n");
        }
    }

    else if (strncmp(input, "echo ", 5) == 0) {
        const char* args = input + 5;
        terminal_writestring(args);
        terminal_writestring("\n");
    }
    else if (strcmp(input, "echo") == 0) {
        terminal_writestring("\n");
    }
    else if (strcmp(input, "uname") == 0) {
        terminal_writestring("Lakos\n");
    }
    else if (strcmp(input, "date") == 0) {
        terminal_writestring("2026-01-10\n");
    }
    else if (strncmp(input, "cat ", 4) == 0) {
        const char* filename = input + 4;
        if (strlen(filename) == 0) {
            terminal_writestring("cat: missing file name\n");
        } else {
            file_t* f = find_file(filename);
            if (f) {
                terminal_writestring(f->content);
            } else {
                terminal_writestring("cat: ");
                terminal_writestring(filename);
                terminal_writestring(": No such file\n");
            }
        }
    }
    else if (strncmp(input, "mkdir ", 6) == 0) {
        const char* dirname = input + 6;
        if (strlen(dirname) == 0) {
            terminal_writestring("mkdir: missing directory name\n");
        } else {
            if (strcmp(current_dir, "/home") == 0) {
                if (home_dir_count >= 10) {
                    terminal_writestring("mkdir: too many directories\n");
                } else {
                    int len = strlen(dirname);
                    if (len >= 32) {
                        terminal_writestring("mkdir: directory name too long\n");
                    } else {
                        strcpy(home_dirs[home_dir_count++], dirname);
                        terminal_writestring("mkdir: created directory '");
                        terminal_writestring(dirname);
                        terminal_writestring("'\n");
                    }
                }
            } else if (strncmp(current_dir, "/home/", 6) == 0) {
                const char* parent = current_dir + 6;
                int parent_len = 0;
                while (parent[parent_len] && parent[parent_len] != '/') parent_len++;
                char parent_name[32];
                if (parent_len >= 32) parent_len = 31;
                for (int k = 0; k < parent_len; k++) parent_name[k] = parent[k];
                parent_name[parent_len] = '\0';
                for (int i = 0; i < home_dir_count; i++) {
                    if (strcmp(home_dirs[i], parent_name) == 0) {
                        if (home_sub_count[i] >= 10) {
                            terminal_writestring("mkdir: too many subdirectories\n");
                        } else {
                            int len = strlen(dirname);
                            if (len >= 32) {
                                terminal_writestring("mkdir: directory name too long\n");
                            } else {
                                strcpy(home_subdirs[i][home_sub_count[i]++], dirname);
                                terminal_writestring("mkdir: created directory '");
                                terminal_writestring(dirname);
                                terminal_writestring("'\n");
                            }
                        }
                        return;
                    }
                }
                terminal_writestring("mkdir: parent directory not found\n");
            } else {
                terminal_writestring("mkdir: can only create directories in /home\n");
            }
        }
    }
    else if (strstr(input, " >> ")) {
        char temp[256];
        strcpy(temp, input);
        char* sep = strstr(temp, " >> ");
        if (sep && strncmp(temp, "echo ", 5) == 0) {
            *sep = '\0';
            const char* text = temp + 5;
            const char* filename = sep + 4;
            file_t* f = find_file(filename);
            if (!f) f = create_file(filename);
            if (f) {
                int len = strlen(text);
                if (f->size + len + 1 < 256) {
                    strcpy(f->content + f->size, text);
                    f->size += len;
                    f->content[f->size++] = '\n';
                    f->content[f->size] = '\0';
                }
            }
        }
    } else if (strcmp(input, "disks") == 0) {
        extern int ata_detect_disks();
        int count = ata_detect_disks();
        terminal_writestring("Detected disks: ");
        for (int i = 0; i < count; i++) {
            terminal_writestring("/dev/hd");
            terminal_putchar('a' + i);
            terminal_writestring("1 ");
        }
        terminal_writestring("\n");
    } else if (strncmp(input, "mount ", 6) == 0) {
        const char* args = input + 6;
        if (strlen(args) == 0) {
            terminal_writestring("mount: missing arguments\nUsage: mount <device> <path>\n");
        } else {
            terminal_writestring("Mounted ");
            terminal_writestring(args);
            terminal_writestring("\n");
        }
    } else {
        if (is_file_in_path(input, pathbin)) {
            terminal_writestring("Executing ");
            terminal_writestring(pathbin);
            terminal_writestring("/");
            terminal_writestring(input);
            terminal_writestring("\n");
        } else {
            terminal_writestring("Error: command '");
            terminal_writestring(input);
            terminal_writestring("' not found.\n");
        }
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
        
        terminal_writestring("C:\\> ");
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
    init_dirs();
    terminal_writestring("\nLakos OS\n");
    terminal_writestring("C:\\> ");
    while(1) {
        if (inb(0x64) & 0x1) {
            uint8_t scancode = inb(0x60);
            if (scancode == 42 || scancode == 54) {
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