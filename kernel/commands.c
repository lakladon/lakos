#include "include/lib.h"
#include "include/users.h"
#include "include/crypt.h"
#include "include/version.h"
#include "include/commands.h"
#include <io.h>

extern void terminal_writestring(const char* s);
extern void terminal_putchar(char c);
extern void terminal_initialize();
extern void* tar_archive;
extern void* tar_lookup(void* archive, const char* filename);
extern void start_gui();
extern int get_current_uid();
extern int get_current_gid();
extern void save_users();
extern int ata_detect_disks();
extern void ata_read_sector(uint8_t drive, uint32_t lba, uint16_t* buffer);
extern void ata_write_sector(uint8_t drive, uint32_t lba, uint16_t* buffer);

// ELF definitions for basic loading
#define EI_NIDENT 16
#define PT_LOAD 1

typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef uint32_t Elf32_Word;

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

// Simple file system simulation
typedef struct {
    char name[32];
    char content[256];
    int size;
} file_t;

static file_t files[10];
static int file_count = 0;

static char dirs[10][32];
static int dir_count = 3; // bin, dev, home

static char home_dirs[10][32];
static int home_dir_count = 0;
static char home_subdirs[10][10][32];
static int home_sub_count[10] = {0};

char current_dir[256] = "/";
static char* pathbin = "/bin";

void init_kernel_commands() {
    strcpy(dirs[0], "bin");
    strcpy(dirs[1], "dev");
    strcpy(dirs[2], "home");
}

static file_t* find_file(const char* name) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, name) == 0) return &files[i];
    }
    return 0;
}

static file_t* create_file(const char* name) {
    if (file_count >= 10) return 0;
    int len = strlen(name);
    if (len >= 32) return 0;
    strcpy(files[file_count].name, name);
    files[file_count].content[0] = '\0';
    files[file_count].size = 0;
    return &files[file_count++];
}

static int is_file_in_path(const char* name, const char* path) {
    if (strcmp(path, "/bin") == 0 && (strcmp(name, "hello") == 0 || strcmp(name, "test") == 0 || strcmp(name, "editor") == 0 || strcmp(name, "calc") == 0)) return 1;
    return 0;
}

static void shutdown() {
    terminal_writestring("Shutting down...\n");
    // For QEMU shutdown
    __asm__ volatile("outw %0, %1" : : "a"((uint16_t)0x2000), "Nd"((uint16_t)0xB004));
    // Fallback
    while (1) {
        __asm__ volatile("hlt");
    }
}

static void reboot() {
    terminal_writestring("Rebooting...\n");
    outb(0x64, 0xFE);
}

static void execute_binary(const char* name) {
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

void kernel_execute_command(const char* input) {
    // Skip leading spaces
    while (*input == ' ') input++;
    if (*input == '\0') return;

    char cmd[64];
    int i = 0;
    while (input[i] && input[i] != ' ' && i < 63) {
        cmd[i] = input[i];
        i++;
    }
    cmd[i] = '\0';
    
    const char* args = input + i;
    while (*args == ' ') args++;

    if (strcmp(cmd, "help") == 0) {
        terminal_writestring("Lakos OS Commands: help, cls, ver, pwd, ls, cd, echo, uname, date, cat, mkdir, disks, read_sector, write_sector, mount, useradd, passwd, login, userdel, crypt, whoami, touch, rm, cp, shutdown, reboot, gui\nAvailable programs: hello, test, editor, calc\n");
    }
    else if (strcmp(cmd, "cls") == 0) {
        terminal_initialize();
    }
    else if (strcmp(cmd, "ver") == 0) {
        terminal_writestring("lakKERNEL ");
        terminal_writestring(KERNEL_VERSION);
        terminal_writestring(" [Kernel Mode]\n");
    }
    else if (strcmp(cmd, "pwd") == 0) {
        terminal_writestring(current_dir);
        terminal_writestring("\n");
    }
    else if (strcmp(cmd, "ls") == 0) {
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
    else if (strcmp(cmd, "cd") == 0) {
        const char* dir = args;
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

    else if (strcmp(cmd, "echo") == 0) {
        terminal_writestring(args);
        terminal_writestring("\n");
    }
    else if (strcmp(cmd, "uname") == 0) {
        terminal_writestring("Lakos\n");
    }
    else if (strcmp(cmd, "date") == 0) {
        terminal_writestring("2026-01-10\n");
    }
    else if (strcmp(cmd, "whoami") == 0) {
        terminal_writestring(current_user);
        terminal_writestring("\n");
    }
    else if (strcmp(cmd, "cat") == 0) {
        const char* filename = args;
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
    else if (strcmp(cmd, "mkdir") == 0) {
        const char* dirname = args;
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
    else if (strcmp(cmd, "touch") == 0) {
        const char* filename = args;
        if (strlen(filename) == 0) {
            terminal_writestring("touch: missing file name\n");
        } else {
            if (!find_file(filename)) {
                create_file(filename);
                terminal_writestring("touch: created file '");
                terminal_writestring(filename);
                terminal_writestring("'\n");
            } else {
                terminal_writestring("touch: file '");
                terminal_writestring(filename);
                terminal_writestring("' already exists\n");
            }
        }
    }
    else if (strcmp(cmd, "rm") == 0) {
        const char* filename = args;
        if (strlen(filename) == 0) {
            terminal_writestring("rm: missing file name\n");
        } else {
            file_t* f = find_file(filename);
            if (f) {
                strcpy(f->name, "");
                f->size = 0;
                terminal_writestring("rm: removed '");
                terminal_writestring(filename);
                terminal_writestring("'\n");
            } else {
                terminal_writestring("rm: ");
                terminal_writestring(filename);
                terminal_writestring(": No such file\n");
            }
        }
    }
    else if (strcmp(cmd, "cp") == 0) {
        const char* p = args;
        char src_name[32];
        int j = 0;
        while (p[j] && p[j] != ' ' && j < 31) {
            src_name[j] = p[j];
            j++;
        }
        src_name[j] = '\0';
        
        const char* dest = p + j;
        while (*dest == ' ') dest++;
        
        if (strlen(src_name) > 0 && strlen(dest) > 0) {
            file_t* s = find_file(src_name);
            if (s) {
                file_t* d = find_file(dest);
                if (!d) d = create_file(dest);
                if (d) {
                    strcpy(d->content, s->content);
                    d->size = s->size;
                    terminal_writestring("cp: copied '");
                    terminal_writestring(src_name);
                    terminal_writestring("' to '");
                    terminal_writestring(dest);
                    terminal_writestring("'\n");
                } else {
                    terminal_writestring("cp: failed to create destination\n");
                }
            } else {
                terminal_writestring("cp: ");
                terminal_writestring(src_name);
                terminal_writestring(": No such file\n");
            }
        } else {
            terminal_writestring("Usage: cp <source> <dest>\n");
        }
    }
    else if (strstr(input, " >> ")) {
        char temp[256];
        strcpy(temp, input);
        char* sep = strstr(temp, " >> ");
        if (sep && strcmp(cmd, "echo") == 0) {
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
    } else if (strcmp(cmd, "disks") == 0) {
        int count = ata_detect_disks();
        terminal_writestring("Detected disks: ");
        for (int i = 0; i < count; i++) {
            terminal_writestring("/dev/hd");
            terminal_putchar('a' + i);
            terminal_writestring("1 ");
        }
        terminal_writestring("\n");
    } else if (strcmp(cmd, "read_sector") == 0) {
        const char* p = args;
        int drive = atoi(p);
        while (*p && *p != ' ') p++;
        if (*p == ' ') p++;
        int lba = atoi(p);
        if (drive >= 0 && lba >= 0) {
            uint16_t buffer[256];
            ata_read_sector(drive, lba, buffer);
            terminal_writestring("Sector data (first 16 words in hex):\n");
            for (int i = 0; i < 16; i++) {
                char hex[10];
                itoa(buffer[i], hex);
                terminal_writestring(hex);
                terminal_writestring(" ");
            }
            terminal_writestring("\n");
        } else {
            terminal_writestring("Usage: read_sector <drive> <lba>\n");
        }
    } else if (strcmp(cmd, "write_sector") == 0) {
        const char* p = args;
        int drive = atoi(p);
        while (*p && *p != ' ') p++;
        if (*p == ' ') p++;
        int lba = atoi(p);
        if (drive >= 0 && lba >= 0) {
            uint16_t buffer[256];
            memset(buffer, 0, 512);
            buffer[0] = 0x4141; // 'AA'
            ata_write_sector(drive, lba, buffer);
            terminal_writestring("Sector written with test data\n");
        } else {
            terminal_writestring("Usage: write_sector <drive> <lba>\n");
        }
    } else if (strcmp(cmd, "mount") == 0) {
        if (strlen(args) == 0) {
            terminal_writestring("mount: missing arguments\nUsage: mount <device> <path>\n");
        } else {
            terminal_writestring("Mounted ");
            terminal_writestring(args);
            terminal_writestring("\n");
        }
    } else if (strcmp(cmd, "useradd") == 0) {
        if (get_current_uid() != 0) {
            terminal_writestring("Permission denied\n");
            return;
        }
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
    } else if (strcmp(cmd, "login") == 0 && strlen(args) == 0) {
        terminal_writestring("Usage: login <username> <password>\n");
    } else if (strcmp(cmd, "passwd") == 0) {
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
    } else if (strcmp(cmd, "login") == 0) {
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
    } else if (strcmp(cmd, "userdel") == 0) {
        if (get_current_uid() != 0) {
            terminal_writestring("Permission denied\n");
            return;
        }
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
    } else if (strcmp(cmd, "crypt") == 0) {
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
    } else if (strcmp(cmd, "shutdown") == 0) {
        shutdown();
    } else if (strcmp(cmd, "reboot") == 0) {
        reboot();
    } else if (strcmp(cmd, "gui") == 0) {
        start_gui();
    } else {
        if (is_file_in_path(cmd, pathbin)) {
            execute_binary(cmd);
            terminal_writestring("Error: command '");
            terminal_writestring(cmd);
            terminal_writestring("' not found.\n");
        }
    }
}            terminal_writestring("Error: command '");
            terminal_writestring(cmd);
            terminal_writestring("' not found.\n");
