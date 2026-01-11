#include <stdint.h>
#include <io.h>
#include "include/users.h"
#include "include/crypt.h"

int strlen(const char* s);

extern void save_users();
extern char current_user[32];

void* memcpy(void* dest, const void* src, unsigned int n);
void* memset(void* s, int c, unsigned int n);

// ELF definitions for basic loading
typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef int32_t Elf32_Sword;
typedef uint32_t Elf32_Word;

#define EI_NIDENT 16
#define PT_LOAD 1

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;
    Elf32_Off e_phoff;
    Elf32_Off e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
} Elf32_Ehdr;

typedef struct {
    Elf32_Word p_type;
    Elf32_Off p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
} Elf32_Phdr;

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

extern void* tar_archive;
extern void* tar_lookup(void* archive, const char* filename);

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

void read_line(char* buffer, int max, int echo) {
    int ptr = 0;
    while (1) {
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



void execute_binary(const char* name) {
    if (!tar_archive) {
        terminal_writestring("No tar archive loaded.\n");
        return;
    }
    if (strlen(name) > 250) {
        terminal_writestring("Binary name too long.\n");
        return;
    }
    char path[256];
    strcpy(path, "bin/");
    strcpy(path + 4, name);

    void* data = tar_lookup(tar_archive, path);
    if (!data) {
        terminal_writestring("Binary not found in tar.\n");
        return;
    }

    Elf32_Ehdr* ehdr = (Elf32_Ehdr*)data;
    if (ehdr->e_ident[0] != 0x7F || ehdr->e_ident[1] != 'E' || ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F') {
        terminal_writestring("Not a valid ELF file.\n");
        return;
    }
    if (ehdr->e_machine != 3) { // i386
        terminal_writestring("Unsupported ELF architecture.\n");
        return;
    }
    if (ehdr->e_entry < 0x200000) {
        terminal_writestring("Invalid entry point address.\n");
        return;
    }

    // Load program headers
    Elf32_Phdr* phdr = (Elf32_Phdr*)((uint8_t*)data + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            if (phdr[i].p_vaddr < 0x200000) {
                terminal_writestring("Invalid load address.\n");
                return;
            }
            if (phdr[i].p_memsz > 0x100000) { // arbitrary limit
                terminal_writestring("Program too large.\n");
                return;
            }
            memcpy((void*)phdr[i].p_vaddr, (uint8_t*)data + phdr[i].p_offset, phdr[i].p_filesz);
            // Zero bss
            memset((void*)(phdr[i].p_vaddr + phdr[i].p_filesz), 0, phdr[i].p_memsz - phdr[i].p_filesz);
        }
    }

    // Set up stack (simple) - allocate in safe memory
    uint32_t* stack = (uint32_t*)0x200000; // arbitrary safe address
    uint32_t* sp = &stack[255];
    *(--sp) = 0; // argv[1] = NULL
    *(--sp) = (uint32_t)name; // argv[0]
    *(--sp) = 1; // argc

    // Call entry point
    typedef int (*entry_t)(int, char**);
    entry_t entry = (entry_t)ehdr->e_entry;
    int ret = entry(1, (char**)sp);

    terminal_writestring("Binary returned: ");
    terminal_putchar('0' + ret);
    terminal_writestring("\n");
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

void* memcpy(void* dest, const void* src, unsigned int n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    while (n--) *d++ = *s++;
    return dest;
}

void* memset(void* s, int c, unsigned int n) {
    unsigned char* p = (unsigned char*)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
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
    if (strcmp(path, "/bin") == 0 && (strcmp(name, "hello") == 0 || strcmp(name, "test") == 0 || strcmp(name, "editor") == 0 || strcmp(name, "calc") == 0)) return 1;
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
        terminal_writestring("Lakos OS Commands: help, cls, ver, pwd, ls, cd, echo, uname, date, cat, mkdir, disks, mount, useradd, passwd, login, userdel, crypt\nAvailable programs: hello, test, editor, calc\n");
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
            terminal_writestring("hello  test  editor  calc\n");
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
    else if (strcmp(input, "calc") == 0) {
        terminal_writestring("Simple Calculator: 2 + 2 = 4\n");
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
    } else if (strncmp(input, "useradd ", 8) == 0) {
        const char* args = input + 8;
        char username[32], password[32];
        char* space = strstr(args, " ");
        if (space) {
            int len = space - args;
            if (len >= 32) len = 31;
            for (int i = 0; i < len; i++) username[i] = args[i];
            username[len] = '\0';
            const char* pass = space + 1;
            int passlen = strlen(pass);
            if (passlen >= 32) passlen = 31;
            for (int i = 0; i < passlen; i++) password[i] = pass[i];
            password[passlen] = '\0';
            if (add_user(username, password)) {
                terminal_writestring("User added: ");
                terminal_writestring(username);
                terminal_writestring("\n");
                save_users();
            } else {
                terminal_writestring("Failed to add user\n");
            }
        } else {
            terminal_writestring("Usage: useradd <username> <password>\n");
        }
    } else if (strncmp(input, "passwd ", 7) == 0) {
        const char* args = input + 7;
        char username[32], newpass[32];
        char* space = strstr(args, " ");
        if (space) {
            int len = space - args;
            if (len >= 32) len = 31;
            for (int i = 0; i < len; i++) username[i] = args[i];
            username[len] = '\0';
            const char* pass = space + 1;
            int passlen = strlen(pass);
            if (passlen >= 32) passlen = 31;
            for (int i = 0; i < passlen; i++) newpass[i] = pass[i];
            newpass[passlen] = '\0';
            int found = 0;
            for (int i = 0; i < user_count; i++) {
                if (strcmp(users[i].username, username) == 0) {
                    strcpy(users[i].password, newpass);
                    terminal_writestring("Password changed for ");
                    terminal_writestring(username);
                    terminal_writestring("\n");
                    found = 1;
                    break;
                }
            }
            if (found) save_users();
            if (!found) {
                terminal_writestring("User not found\n");
            }
        } else {
            terminal_writestring("Usage: passwd <username> <newpassword>\n");
        }
    } else if (strncmp(input, "login ", 6) == 0) {
        const char* args = input + 6;
        char username[32], password[32];
        char* space = strstr(args, " ");
        if (space) {
            int len = space - args;
            if (len >= 32) len = 31;
            for (int i = 0; i < len; i++) username[i] = args[i];
            username[len] = '\0';
            const char* pass = space + 1;
            int passlen = strlen(pass);
            if (passlen >= 32) passlen = 31;
            for (int i = 0; i < passlen; i++) password[i] = pass[i];
            password[passlen] = '\0';
            if (authenticate_user(username, password)) {
                terminal_writestring("Logged in as ");
                terminal_writestring(username);
                terminal_writestring("\n");
            } else {
                terminal_writestring("Invalid username or password\n");
            }
        } else {
            terminal_writestring("Usage: login <username> <password>\n");
        }
    } else if (strncmp(input, "userdel ", 8) == 0) {
        const char* args = input + 8;
        if (strlen(args) > 0) {
            if (delete_user(args)) {
                terminal_writestring("User deleted: ");
                terminal_writestring(args);
                terminal_writestring("\n");
                save_users();
            } else {
                terminal_writestring("Failed to delete user\n");
            }
        } else {
            terminal_writestring("Usage: userdel <username>\n");
        }
    } else if (strncmp(input, "crypt ", 6) == 0) {
        const char* args = input + 6;
        if (args[0] == '-' && (args[1] == 'e' || args[1] == 'd') && args[2] == ' ') {
            char mode = args[1];
            const char* p = args + 3;
            char key[32];
            int k = 0;
            while (*p && *p != ' ' && k < 31) {
                key[k++] = *p++;
            }
            key[k] = '\0';
            if (*p == ' ') p++;
            char text[MAX_PASS_LEN];
            int t = 0;
            while (*p && t < MAX_PASS_LEN - 1) {
                text[t++] = *p++;
            }
            text[t] = '\0';
            char result[MAX_PASS_LEN];
            if (mode == 'e') {
                encrypt_password(text, key, result);
                terminal_writestring("Encrypted: ");
                terminal_writestring(result);
                terminal_writestring("\n");
            } else if (mode == 'd') {
                decrypt_password(text, key, result);
                terminal_writestring("Decrypted: ");
                terminal_writestring(result);
                terminal_writestring("\n");
            }
        } else {
            terminal_writestring("Usage: crypt -e <key> <password> or crypt -d <key> <encrypted>\n");
        }
    } else {
        if (is_file_in_path(input, pathbin)) {
            execute_binary(input);
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

        terminal_writestring("LakOS>");
        terminal_writestring(current_user);
        terminal_writestring(" ");
        terminal_writestring(current_dir);
        terminal_writestring(" ");
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
    init_dirs();
    terminal_writestring("Dirs done\n");
    init_users();
    terminal_writestring("Users done\n");
    terminal_writestring("\n");
    terminal_writestring(" _         _    ___  ___ \n");
    terminal_writestring("| |   ___ | |__| . |/ __>\n");
    terminal_writestring("| |_ <_> || / /| | |\\__ \\\n");
    terminal_writestring("|___|<___||_\\_\\`___'<___/\n");
    terminal_writestring("\nWelcome to Lakos OS\n");

    terminal_writestring("Shell initialized, starting login\n");

    // Login
    char username[32];
    char password[32];
    while (1) {
        terminal_writestring("Login: ");
        read_line(username, 31, 1);
        terminal_writestring("Password: ");
        read_line(password, 31, 0);
        terminal_putchar('\n');
        if (authenticate_user(username, password)) {
            break;
        } else {
            terminal_writestring("Invalid login\n");
        }
    }

    terminal_writestring("Login successful\n");
    terminal_writestring("LakOS>");
    terminal_writestring(current_user);
    terminal_writestring(" / ");
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