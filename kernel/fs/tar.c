#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "include/lib.h"

extern void terminal_writestring(const char* s);

struct tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
} __attribute__((packed));

// Fixed function get_size for octal
static unsigned int get_size(const char *in) {
    unsigned int size = 0;
    int i = 0;
    // Skip leading spaces
    while (i < 12 && in[i] == ' ') i++;
    // Parse octal digits
    for (; i < 12 && in[i] >= '0' && in[i] <= '7'; i++) {
        size = size * 8 + (in[i] - '0');
    }
    return size;
}

// Function to check if a path exists in the tar archive
static int tar_path_exists(void* archive, const char* path) {
    unsigned char* ptr = (unsigned char*)archive;
    
    while (ptr[0] != '\0') {
        struct tar_header* header = (struct tar_header*)ptr;
        
        // Check if this entry matches the path
        if (strcmp(header->name, path) == 0) {
            return 1; // Found exact match
        }
        
        // Check if this entry is a parent directory of the path
        int path_len = strlen(path);
        int name_len = strlen(header->name);
        if (name_len < path_len && 
            strncmp(header->name, path, name_len) == 0 &&
            path[name_len] == '/') {
            return 1; // Found parent directory
        }
        
        unsigned int size = get_size(header->size);
        ptr += ((size + 511) / 512 + 1) * 512;
    }
    return 0; // Not found
}

// Function to get all directories from tar archive
static int tar_add_directory(char directories[][256], int* count, const char* name, int len) {
    if (len <= 0 || *count >= 100) {
        return 0;
    }

    char temp[256];
    if (len >= 256) {
        len = 255;
    }
    strncpy(temp, name, len);
    temp[len] = '\0';

    // Normalize: trim leading "./" and trailing '/'
    if (temp[0] == '.' && temp[1] == '/') {
        int shift = 2;
        int i = 0;
        while (temp[shift + i] != '\0') {
            temp[i] = temp[shift + i];
            i++;
        }
        temp[i] = '\0';
    }

    if (temp[0] == '\0') {
        return 0;
    }

    int tlen = strlen(temp);
    while (tlen > 0 && temp[tlen - 1] == '/') {
        temp[tlen - 1] = '\0';
        tlen--;
    }
    if (tlen == 0) {
        return 0;
    }

    // Avoid duplicates
    for (int i = 0; i < *count; i++) {
        if (strcmp(directories[i], temp) == 0) {
            return 0;
        }
    }

    strcpy(directories[*count], temp);
    (*count)++;
    return 1;
}

static void tar_add_parent_directories(char directories[][256], int* count, const char* name, int len) {
    if (len <= 0) {
        return;
    }

    int limit = len;
    if (limit >= 256) {
        limit = 255;
    }

    char temp[256];
    strncpy(temp, name, limit);
    temp[limit] = '\0';

    // Add each parent directory for nested paths
    for (int i = 0; temp[i] != '\0'; i++) {
        if (temp[i] == '/') {
            tar_add_directory(directories, count, temp, i);
        }
    }
    tar_add_directory(directories, count, temp, strlen(temp));
}

static void tar_get_all_directories(void* archive, char directories[][256], int* count) {
    unsigned char* ptr = (unsigned char*)archive;
    *count = 0;

    while (ptr[0] != '\0') {
        struct tar_header* header = (struct tar_header*)ptr;

        if (header->name[0] != '\0') {
            int name_len = strlen(header->name);
            if (header->typeflag == '5' || header->typeflag == 'D') {
                tar_add_parent_directories(directories, count, header->name, name_len);
            } else {
                char* last_slash = strrchr(header->name, '/');
                if (last_slash != NULL) {
                    int dir_len = last_slash - header->name;
                    tar_add_parent_directories(directories, count, header->name, dir_len);
                }
            }
        }

        unsigned int size = get_size(header->size);
        ptr += ((size + 511) / 512 + 1) * 512;
    }
}

// Эту функцию ищет линковщик для shell.c!
void tar_list_files(void* archive) {
    unsigned char* ptr = (unsigned char*)archive;
    
    // Если указатель на архив пустой, выходим
    if (!ptr) return;

    while (ptr[0] != '\0') {
        struct tar_header* header = (struct tar_header*)ptr;

        // Если это обычный файл (type '0' или '\0')
        if (header->name[0] != '\0') {
            // Здесь должен быть вызов твоей функции печати на экран
            terminal_writestring(header->name);
            terminal_writestring("\n");
        }

        unsigned int size = get_size(header->size);
        ptr += ((size + 511) / 512 + 1) * 512;
    }
}

// New function to check if a path exists in tar archive
int tar_check_path_exists(void* archive, const char* path) {
    return tar_path_exists(archive, path);
}

// New function to get all directories from tar archive
void tar_get_directories(void* archive, char directories[][256], int* count) {
    tar_get_all_directories(archive, directories, count);
}

// Твоя функция поиска
void* tar_lookup(void* archive, const char* filename) {
    unsigned char* ptr = (unsigned char*)archive;
    char buf[16];
    char cbuf[2];
    terminal_writestring("Tar lookup for: ");
    terminal_writestring(filename);
    terminal_writestring("\n");

    while (ptr[0] != '\0') {
        struct tar_header* header = (struct tar_header*)ptr;
        terminal_writestring("Checking header: '");
        terminal_writestring(header->name);
        terminal_writestring("' against '");
        terminal_writestring(filename);
        terminal_writestring("'\n");

        int match = 1;
        int i;
        for (i = 0; filename[i] != '\0'; i++) {
            if (header->name[i] != filename[i]) {
                terminal_writestring("Mismatch at pos ");
                itoa(i, buf);
                terminal_writestring(buf);
                terminal_writestring(": header '");
                cbuf[0] = header->name[i];
                cbuf[1] = '\0';
                terminal_writestring(cbuf);
                terminal_writestring("' vs filename '");
                cbuf[0] = filename[i];
                terminal_writestring(cbuf);
                terminal_writestring("'\n");
                match = 0;
                break;
            }
        }
        if (match) {
            terminal_writestring("Prefix match, checking end at pos ");
            itoa(i, buf);
            terminal_writestring(buf);
            terminal_writestring(": header char '");
            cbuf[0] = header->name[i];
            terminal_writestring(cbuf);
            terminal_writestring("'\n");
            if (header->name[i] == '\0' || header->name[i] == ' ') {
                terminal_writestring("Match found!\n");
                return (void*)(ptr + 512);
            } else {
                terminal_writestring("End char not null or space, no match\n");
            }
        }

        unsigned int size = get_size(header->size);
        itoa(size, buf);
        terminal_writestring("Size: ");
        terminal_writestring(buf);
        terminal_writestring(", advancing\n");
        ptr += ((size + 511) / 512 + 1) * 512;
    }
    terminal_writestring("No match found.\n");
    return NULL;
}

// Get file size for a given path in the tar archive
int tar_get_file_size(void* archive, const char* filename) {
    unsigned char* ptr = (unsigned char*)archive;

    while (ptr[0] != '\0') {
        struct tar_header* header = (struct tar_header*)ptr;
        if (strcmp(header->name, filename) == 0) {
            return (int)get_size(header->size);
        }

        unsigned int size = get_size(header->size);
        ptr += ((size + 511) / 512 + 1) * 512;
    }

    return -1;
}