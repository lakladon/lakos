#include "include/lib.h"
#include "include/users.h"
#include "include/crypt.h"
#include "include/version.h"
#include "include/commands.h"
#include "drivers/io.h"
extern void terminal_writestring(const char* s);
extern void terminal_putchar(char c);
extern void terminal_initialize();
extern void* tar_archive;
extern void* tar_lookup(void* archive, const char* filename);
extern int tar_check_path_exists(void* archive, const char* path);
extern void tar_get_directories(void* archive, char directories[][256], int* count);
extern void tar_list_directory(void* archive, const char* dirpath);
extern int tar_get_file_size(void* archive, const char* filename);
#define FILE_SIZE_THRESHOLD 1024
extern void start_gui();
extern int get_current_uid();
extern int get_current_gid();
extern void save_users();
extern int ata_detect_disks();
extern void ata_read_sector(uint8_t drive, uint32_t lba, uint16_t* buffer);
extern void ata_write_sector(uint8_t drive, uint32_t lba, uint16_t* buffer);
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
typedef struct {
    char name[32];
    char content[256];
    int size;
} file_t;
static file_t files[10];
static int file_count = 0;
static char dirs[10][32];
static int dir_count = 0;
static char home_dirs[10][32];
static int home_dir_count = 0;
static char home_subdirs[10][10][32];
static int home_sub_count[10] = {0};
char current_dir[256] = "/";
static char* pathbin = "/bin";
char get_char();
void grep(const char* args);
void execute_command_with_output(const char* command, char* output, int output_size);
void execute_command_with_input(const char* command, const char* input);
void grep_with_output(const char* pattern, const char* filename, char* output, int output_size);
void grep_with_input(const char* pattern, const char* input);
static file_t* find_file(const char* name);
static void append_capture(char* output, int output_size, const char* text) {
    if (!output || output_size <= 0 || !text) return;
    int len = strlen(output);
    int add = strlen(text);
    if (len >= output_size - 1) return;
    if (len + add >= output_size) {
        add = output_size - len - 1;
    }
    if (add > 0) {
        strncat(output, text, add);
    }
}
static void terminal_write_n(const char* s, int n) {
    if (!s || n <= 0) return;
    for (int i = 0; i < n; i++) {
        terminal_putchar(s[i]);
    }
}
static void append_capture_n(char* output, int output_size, const char* text, int n) {
    if (!output || output_size <= 0 || !text || n <= 0) return;
    for (int i = 0; i < n; i++) {
        int len = strlen(output);
        if (len >= output_size - 1) break;
        output[len] = text[i];
        output[len + 1] = '\0';
    }
}
static void print_highlighted_line(const char* line, const char* pattern) {
    if (!line) return;
    if (!pattern || pattern[0] == '\0') {
        terminal_writestring(line);
        terminal_putchar('\n');
        return;
    }
    int pat_len = strlen(pattern);
    const char* p = line;
    while (1) {
        const char* match = strstr(p, pattern);
        if (!match) {
            terminal_writestring(p);
            break;
        }
        terminal_write_n(p, (int)(match - p));
        terminal_writestring("\033[31m");
        terminal_write_n(match, pat_len);
        terminal_writestring("\033[0m");
        p = match + pat_len;
    }
    terminal_putchar('\n');
}
static void append_highlighted_line(char* output, int output_size, const char* line, const char* pattern) {
    if (!output || output_size <= 0 || !line) return;
    if (!pattern || pattern[0] == '\0') {
        append_capture(output, output_size, line);
        append_capture(output, output_size, "\n");
        return;
    }
    int pat_len = strlen(pattern);
    const char* p = line;
    while (1) {
        const char* match = strstr(p, pattern);
        if (!match) {
            append_capture(output, output_size, p);
            break;
        }
        append_capture_n(output, output_size, p, (int)(match - p));
        append_capture(output, output_size, "\033[31m");
        append_capture_n(output, output_size, match, pat_len);
        append_capture(output, output_size, "\033[0m");
        p = match + pat_len;
    }
    append_capture(output, output_size, "\n");
}
static file_t* find_file(const char* name);
static file_t* create_file(const char* name);
static void shutdown();
static void reboot();
#include "comand/help.c"
#include "comand/cls.c"
#include "comand/ver.c"
#include "comand/pwd.c"
#include "comand/ls.c"
#include "comand/cd.c"
#include "comand/cat.c"
#include "comand/echo.c"
#include "comand/uname.c"
#include "comand/date.c"
#include "comand/whoami.c"
#include "comand/mkdir.c"
#include "comand/touch.c"
#include "comand/rm.c"
#include "comand/cp.c"
#include "comand/disks.c"
#include "comand/read_sector.c"
#include "comand/write_sector.c"
#include "comand/mount.c"
#include "comand/useradd.c"
#include "comand/passwd.c"
#include "comand/login.c"
#include "comand/userdel.c"
#include "comand/crypt.c"
#include "comand/shutdown.c"
#include "comand/reboot.c"
#include "comand/gui.c"
#include "comand/grep.c"
#include "comand/calc.c"
#include "comand/asm.c"
#include "comand/net.c"
#include "comand/colorb.c"
#include "comand/lsh.c"
void init_kernel_commands() {
    if (tar_archive) {
        char tar_dirs[100][256];
        int tar_count = 0;
        tar_get_directories(tar_archive, tar_dirs, &tar_count);
        for (int i = 0; i < tar_count && dir_count < 10; i++) {
            if (tar_dirs[i][0] == '\0') {
                continue;
            }
            char* slash = strchr(tar_dirs[i], '/');
            int name_len = 0;
            if (slash) {
                name_len = slash - tar_dirs[i];
            } else {
                name_len = strlen(tar_dirs[i]);
            }
            if (name_len <= 0 || name_len >= (int)sizeof(dirs[0])) {
                continue;
            }
            char top_name[32];
            strncpy(top_name, tar_dirs[i], name_len);
            top_name[name_len] = '\0';
            int exists = 0;
            for (int d = 0; d < dir_count; d++) {
                if (strcmp(dirs[d], top_name) == 0) {
                    exists = 1;
                    break;
                }
            }
            if (!exists) {
                strcpy(dirs[dir_count++], top_name);
            }
        }
    } else {
        strcpy(dirs[dir_count++], "bin");
        strcpy(dirs[dir_count++], "dev");
        strcpy(dirs[dir_count++], "home");
    }
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
    if (!tar_archive) {
        return 0;
    }
    const char* clean_path = path;
    if (clean_path[0] == '/' && clean_path[1] != '\0') {
        clean_path++;
    }
    char tar_path[256];
    if (clean_path[0] == '\0') {
        snprintf(tar_path, sizeof(tar_path), "%s", name);
    } else {
        snprintf(tar_path, sizeof(tar_path), "%s/%s", clean_path, name);
    }
    void* data = tar_lookup(tar_archive, tar_path);
    return data != 0;
}
static void shutdown() {
    terminal_writestring("Shutting down...\n");
    __asm__ volatile("outw %0, %1" : : "a"((uint16_t)0x2000), "Nd"((uint16_t)0xB004));
    while (1) {
        __asm__ volatile("hlt");
    }
}
static void reboot() {
    terminal_writestring("Rebooting...\n");
    outb(0x64, 0xFE);
}
static char user_input_buffer[256];
static int user_input_pos = 0;
char get_char() {
    while (1) {
        if (inb(0x64) & 0x1) {
            uint8_t scancode = inb(0x60);
            if (!(scancode & 0x80)) { 
                char c = 0;
                switch (scancode) {
                    case 28: 
                        return '\n';
                    case 14: 
                        if (user_input_pos > 0) {
                            user_input_pos--;
                            terminal_putchar('\b');
                            terminal_putchar(' ');
                            terminal_putchar('\b');
                        }
                        break;
                    case 57: 
                        c = ' ';
                        break;
                    default:
                        if (scancode >= 2 && scancode <= 11) { 
                            c = '1' + (scancode - 2);
                        } else if (scancode == 12) { 
                            c = '-';
                        } else if (scancode == 13) { 
                            c = '=';
                        } else if (scancode >= 16 && scancode <= 25) { 
                            c = 'q' + (scancode - 16);
                        } else if (scancode >= 30 && scancode <= 38) { 
                            c = 'a' + (scancode - 30);
                        } else if (scancode >= 44 && scancode <= 50) { 
                            c = 'z' + (scancode - 44);
                        }
                        break;
                }
                if (c >= 32 && c < 127 && user_input_pos < 255) {
                    user_input_buffer[user_input_pos++] = c;
                    terminal_putchar(c);
                }
            }
        }
    }
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
    if (ehdr->e_machine != 3) { 
        terminal_writestring("Unsupported ELF architecture.\n");
        return;
    }
    if (ehdr->e_entry < 0x100000) {
        terminal_writestring("Invalid entry point address.\n");
        return;
    }
    Elf32_Phdr* phdr = (Elf32_Phdr*)((uint8_t*)data + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            if (phdr[i].p_vaddr < 0x100000) {
                terminal_writestring("Invalid load address.\n");
                return;
            }
            if (phdr[i].p_memsz > 0x100000) { 
                terminal_writestring("Program too large.\n");
                return;
            }
            memcpy((void*)phdr[i].p_vaddr, (uint8_t*)data + phdr[i].p_offset, phdr[i].p_filesz);
            memset((void*)(phdr[i].p_vaddr + phdr[i].p_filesz), 0, phdr[i].p_memsz - phdr[i].p_filesz);
        }
    }
    uint32_t* stack = (uint32_t*)0x200000; 
    uint32_t* sp = &stack[255];
    *(--sp) = 0; 
    *(--sp) = (uint32_t)name; 
    *(--sp) = 1; 
    typedef int (*entry_t)(int, char**);
    entry_t entry = (entry_t)ehdr->e_entry;
    int ret = entry(1, (char**)sp);
    terminal_writestring("Binary returned: ");
    terminal_putchar('0' + ret);
    terminal_writestring("\n");
}
void kernel_execute_command(const char* input) {
    const char* pipe_pos = strchr(input, '|');
    if (pipe_pos) {
        char left_cmd[256];
        char right_cmd[256];
        int left_len = pipe_pos - input;
        strncpy(left_cmd, input, left_len);
        left_cmd[left_len] = '\0';
        while (left_len > 0 && left_cmd[left_len - 1] == ' ') {
            left_cmd[--left_len] = '\0';
        }
        const char* right_start = pipe_pos + 1;
        while (*right_start == ' ') right_start++;
        strcpy(right_cmd, right_start);
        int right_len = strlen(right_cmd);
        while (right_len > 0 && right_cmd[right_len - 1] == ' ') {
            right_cmd[--right_len] = '\0';
        }
        char buffer[1024];
        execute_command_with_output(left_cmd, buffer, sizeof(buffer));
        execute_command_with_input(right_cmd, buffer);
        return;
    }
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
    if (strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        cmd_help("");
        return;
    }
    if (strcmp(args, "--help") == 0 || strcmp(args, "-h") == 0) {
        cmd_man(cmd);
        return;
    }
    if (strcmp(cmd, "help") == 0) {
        cmd_help(args);
    }
    else if (strcmp(cmd, "man") == 0) {
        cmd_man(args);
    }
    else if (strcmp(cmd, "cls") == 0) {
        cmd_cls(args);
    }
    else if (strcmp(cmd, "ver") == 0) {
        cmd_ver(args);
    }
    else if (strcmp(cmd, "pwd") == 0) {
        cmd_pwd(args);
    }
    else if (strcmp(cmd, "ls") == 0) {
        cmd_ls(args);
    }
    else if (strcmp(cmd, "cd") == 0) {
        cmd_cd(args);
    }
    else if (strcmp(cmd, "cat") == 0) {
        cmd_cat(args);
    }
    else if (strcmp(cmd, "echo") == 0) {
        cmd_echo(args);
    }
    else if (strcmp(cmd, "uname") == 0) {
        cmd_uname(args);
    }
    else if (strcmp(cmd, "date") == 0) {
        cmd_date(args);
    }
    else if (strcmp(cmd, "whoami") == 0) {
        cmd_whoami(args);
    }
    else if (strcmp(cmd, "mkdir") == 0) {
        cmd_mkdir(args);
    }
    else if (strcmp(cmd, "touch") == 0) {
        cmd_touch(args);
    }
    else if (strcmp(cmd, "rm") == 0) {
        cmd_rm(args);
    }
    else if (strcmp(cmd, "cp") == 0) {
        cmd_cp(args);
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
        cmd_disks(args);
    } else if (strcmp(cmd, "read_sector") == 0) {
        cmd_read_sector(args);
    } else if (strcmp(cmd, "write_sector") == 0) {
        cmd_write_sector(args);
    } else if (strcmp(cmd, "mount") == 0) {
        cmd_mount(args);
    } else if (strcmp(cmd, "useradd") == 0) {
        cmd_useradd(args);
    } else if (strcmp(cmd, "login") == 0) {
        cmd_login(args);
    } else if (strcmp(cmd, "passwd") == 0) {
        cmd_passwd(args);
    } else if (strcmp(cmd, "userdel") == 0) {
        cmd_userdel(args);
    } else if (strcmp(cmd, "crypt") == 0) {
        cmd_crypt(args);
    } else if (strcmp(cmd, "shutdown") == 0) {
        cmd_shutdown(args);
    } else if (strcmp(cmd, "reboot") == 0) {
        cmd_reboot(args);
    } else if (strcmp(cmd, "gui") == 0) {
        cmd_gui(args);
    } else if (strcmp(cmd, "grep") == 0) {
        cmd_grep(args);
    } else if (strcmp(cmd, "calc") == 0) {
        cmd_calc(args);
    } else if (strcmp(cmd, "asm") == 0) {
        cmd_asm(args);
    } else if (strcmp(cmd, "ifconfig") == 0) {
        cmd_ifconfig(args);
    } else if (strcmp(cmd, "ping") == 0) {
        cmd_ping(args);
    } else if (strcmp(cmd, "wget") == 0) {
        cmd_wget(args);
    } else if (strcmp(cmd, "netstat") == 0) {
        cmd_netstat(args);
    } else if (strcmp(cmd, "arp") == 0) {
        cmd_arp(args);
    } else if (strcmp(cmd, "nethelp") == 0) {
        cmd_nethelp(args);
    } else if (strcmp(cmd, "colorb") == 0) {
        cmd_colorb(args);
    } else if (strcmp(cmd, "lsh") == 0) {
        cmd_lsh(args);
    } else {
        if (is_file_in_path(cmd, pathbin)) {
            execute_binary(cmd);
        } else {
            terminal_writestring("Error: command '");
            terminal_writestring(cmd);
            terminal_writestring("' not found.\n");
        }
    }
}
void grep(const char* args) {
    if (strlen(args) == 0) {
        terminal_writestring("grep: missing pattern\n");
        return;
    }
    char pattern[256];
    char filename[256];
    const char* p = args;
    int j = 0;
    while (*p && *p != ' ' && j < 255) {
        pattern[j++] = *p++;
    }
    pattern[j] = '\0';
    while (*p == ' ') p++;
    strcpy(filename, p);
    if (strlen(pattern) == 0 || strlen(filename) == 0) {
        terminal_writestring("grep: usage: grep <pattern> <filename>\n");
        return;
    }
    if (tar_archive) {
        char tar_path[256];
        if (filename[0] == '/') {
            strcpy(tar_path, filename + 1);
        } else if (strcmp(current_dir, "/") == 0) {
            strcpy(tar_path, filename);
        } else {
            strcpy(tar_path, current_dir + 1);
            if (tar_path[strlen(tar_path) - 1] != '/') {
                strcat(tar_path, "/");
            }
            strcat(tar_path, filename);
        }
        void* data = tar_lookup(tar_archive, tar_path);
        int size = tar_get_file_size(tar_archive, tar_path);
        if (data && size >= 0) {
            char* bytes = (char*)data;
            int line_start = 0;
            int found = 0;
            for (int i = 0; i <= size; i++) {
                if (bytes[i] == '\n' || i == size) {
                    int line_len = i - line_start;
                    if (line_len > 0) {
                        char line[256];
                        if (line_len >= 256) line_len = 255;
                        strncpy(line, bytes + line_start, line_len);
                        line[line_len] = '\0';
                        if (strstr(line, pattern)) {
                            print_highlighted_line(line, pattern);
                            found = 1;
                        }
                    }
                    line_start = i + 1;
                }
            }
            if (!found) {
                terminal_writestring("No matches found\n");
            }
            return;
        }
    }
    file_t* f = find_file(filename);
    if (f) {
        int line_start = 0;
        int found = 0;
        for (int i = 0; i <= f->size; i++) {
            if (f->content[i] == '\n' || i == f->size) {
                int line_len = i - line_start;
                if (line_len > 0) {
                    char line[256];
                    if (line_len >= 256) line_len = 255;
                    strncpy(line, f->content + line_start, line_len);
                    line[line_len] = '\0';
                    if (strstr(line, pattern)) {
                        print_highlighted_line(line, pattern);
                        found = 1;
                    }
                }
                line_start = i + 1;
            }
        }
        if (!found) {
            terminal_writestring("No matches found\n");
        }
    } else {
        terminal_writestring("grep: ");
        terminal_writestring(filename);
        terminal_writestring(": No such file\n");
    }
}
void execute_command_with_output(const char* command, char* output, int output_size) {
    if (!output || output_size <= 0) {
        return;
    }
    output[0] = '\0';
    terminal_capture_begin(output, output_size);
    kernel_execute_command(command);
    terminal_capture_end();
}
void execute_command_with_input(const char* command, const char* input) {
    char cmd[64];
    int i = 0;
    while (command[i] && command[i] != ' ' && i < 63) {
        cmd[i] = command[i];
        i++;
    }
    cmd[i] = '\0';
    const char* args = command + i;
    while (*args == ' ') args++;
    if (strcmp(cmd, "grep") == 0) {
        char pattern[256];
        const char* p = args;
        int j = 0;
        while (*p && *p != ' ' && j < 255) {
            pattern[j++] = *p++;
        }
        pattern[j] = '\0';
        grep_with_input(pattern, input);
    } else {
        kernel_execute_command(command);
    }
}
void grep_with_output(const char* pattern, const char* filename, char* output, int output_size) {
    if (tar_archive) {
        char tar_path[256];
        if (filename[0] == '/') {
            strcpy(tar_path, filename + 1);
        } else if (strcmp(current_dir, "/") == 0) {
            strcpy(tar_path, filename);
        } else {
            strcpy(tar_path, current_dir + 1);
            if (tar_path[strlen(tar_path) - 1] != '/') {
                strcat(tar_path, "/");
            }
            strcat(tar_path, filename);
        }
        void* data = tar_lookup(tar_archive, tar_path);
        int size = tar_get_file_size(tar_archive, tar_path);
        if (data && size >= 0) {
            char* bytes = (char*)data;
            int line_start = 0;
            int output_pos = 0;
            int found = 0;
            for (int i = 0; i <= size; i++) {
                if (bytes[i] == '\n' || i == size) {
                    int line_len = i - line_start;
                    if (line_len > 0) {
                        char line[256];
                        if (line_len >= 256) line_len = 255;
                        strncpy(line, bytes + line_start, line_len);
                        line[line_len] = '\0';
                        if (strstr(line, pattern)) {
                            append_highlighted_line(output, output_size, line, pattern);
                            output_pos = strlen(output);
                            found = 1;
                        }
                    }
                    line_start = i + 1;
                }
            }
            if (!found) {
                strcpy(output, "No matches found\n");
            } else {
                output[output_pos] = '\0';
            }
            return;
        }
    }
    file_t* f = find_file(filename);
    if (f) {
        int line_start = 0;
        int output_pos = 0;
        int found = 0;
        for (int i = 0; i <= f->size; i++) {
            if (f->content[i] == '\n' || i == f->size) {
                int line_len = i - line_start;
                if (line_len > 0) {
                    char line[256];
                    if (line_len >= 256) line_len = 255;
                    strncpy(line, f->content + line_start, line_len);
                    line[line_len] = '\0';
                    if (strstr(line, pattern)) {
                        append_highlighted_line(output, output_size, line, pattern);
                        output_pos = strlen(output);
                        found = 1;
                    }
                }
                line_start = i + 1;
            }
        }
        if (!found) {
            strcpy(output, "No matches found\n");
        } else {
            output[output_pos] = '\0';
        }
    } else {
        strcpy(output, "grep: file not found\n");
    }
}
void grep_with_input(const char* pattern, const char* input) {
    if (!input || !pattern) {
        terminal_writestring("grep: invalid input\n");
        return;
    }
    int line_start = 0;
    int found = 0;
    for (int i = 0; input[i] != '\0'; i++) {
        if (input[i] == '\n') {
            int line_len = i - line_start;
            if (line_len > 0) {
                char line[256];
                if (line_len >= 256) line_len = 255;
                strncpy(line, input + line_start, line_len);
                line[line_len] = '\0';
                if (strstr(line, pattern)) {
                    print_highlighted_line(line, pattern);
                    found = 1;
                }
            }
            line_start = i + 1;
        }
    }
    int last_line_len = strlen(input) - line_start;
    if (last_line_len > 0) {
        char line[256];
        if (last_line_len >= 256) last_line_len = 255;
        strncpy(line, input + line_start, last_line_len);
        line[last_line_len] = '\0';
        if (strstr(line, pattern)) {
            print_highlighted_line(line, pattern);
            found = 1;
        }
    }
    if (!found) {
        terminal_writestring("No matches found\n");
    }
}
