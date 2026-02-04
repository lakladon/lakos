#include <stdint.h>
#include <stddef.h>
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
    if (!path) {
        return 0;
    }

    // Normalize path: strip leading '/', trim trailing '/'
    const char* norm = path;
    if (norm[0] == '/') {
        norm++;
    }

    char path_buf[256];
    int path_len = strlen(norm);
    while (path_len > 0 && norm[path_len - 1] == '/') {
        path_len--;
    }
    if (path_len >= 255) {
        path_len = 255;
    }
    strncpy(path_buf, norm, path_len);
    path_buf[path_len] = '\0';

    if (path_buf[0] == '\0') {
        return 1; // root always exists
    }

    unsigned char* ptr = (unsigned char*)archive;
    
    while (ptr[0] != '\0') {
        struct tar_header* header = (struct tar_header*)ptr;
        
        // Check if this entry matches the path
        if (strcmp(header->name, path_buf) == 0) {
            return 1; // Found exact match
        }

        // Allow header names with trailing '/'
        int name_len = strlen(header->name);
        if (name_len > 0 && header->name[name_len - 1] == '/' &&
            name_len - 1 == path_len &&
            strncmp(header->name, path_buf, path_len) == 0) {
            return 1;
        }
        
        // Check if this entry is a parent directory of the path
        name_len = strlen(header->name);
        if (name_len < path_len && 
            strncmp(header->name, path_buf, name_len) == 0 &&
            path_buf[name_len] == '/') {
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

static int tar_add_list_entry(char entries[][256], int* count, const char* name) {
    if (!name || name[0] == '\0' || *count >= 100) {
        return 0;
    }

    for (int i = 0; i < *count; i++) {
        if (strcmp(entries[i], name) == 0) {
            return 0;
        }
    }

    strncpy(entries[*count], name, 255);
    entries[*count][255] = '\0';
    (*count)++;
    return 1;
}

void tar_list_directory(void* archive, const char* dirpath) {
    unsigned char* ptr = (unsigned char*)archive;
    if (!ptr) {
        return;
    }

    const char* norm = dirpath ? dirpath : "";
    if (norm[0] == '/') {
        norm++;
    }

    char base[256];
    int base_len = strlen(norm);
    while (base_len > 0 && norm[base_len - 1] == '/') {
        base_len--;
    }
    if (base_len >= 255) {
        base_len = 255;
    }
    strncpy(base, norm, base_len);
    base[base_len] = '\0';

    char prefix[256];
    prefix[0] = '\0';
    if (base[0] != '\0') {
        strcpy(prefix, base);
        if (prefix[strlen(prefix) - 1] != '/') {
            strcat(prefix, "/");
        }
    }
    int prefix_len = strlen(prefix);

    char entries[100][256];
    int entry_count = 0;

    while (ptr[0] != '\0') {
        struct tar_header* header = (struct tar_header*)ptr;

        if (header->name[0] != '\0') {
            const char* name = header->name;
            int name_len = strlen(name);
            int name_is_dir = (header->typeflag == '5' || header->typeflag == 'D' ||
                               (name_len > 0 && name[name_len - 1] == '/'));

            if (prefix_len == 0) {
                const char* slash = strchr(name, '/');
                int comp_len = slash ? (int)(slash - name) : name_len;
                if (comp_len > 0) {
                    char entry[256];
                    if (comp_len >= 255) comp_len = 255;
                    strncpy(entry, name, comp_len);
                    entry[comp_len] = '\0';

                    int is_dir = (slash != NULL) || name_is_dir;
                    if (is_dir && strlen(entry) < 255) {
                        strcat(entry, "/");
                    }
                    tar_add_list_entry(entries, &entry_count, entry);
                }
            } else if (strncmp(name, prefix, prefix_len) == 0) {
                const char* rest = name + prefix_len;
                if (rest[0] != '\0') {
                    const char* slash = strchr(rest, '/');
                    int comp_len = slash ? (int)(slash - rest) : (int)strlen(rest);
                    if (comp_len > 0) {
                        char entry[256];
                        if (comp_len >= 255) comp_len = 255;
                        strncpy(entry, rest, comp_len);
                        entry[comp_len] = '\0';

                        int is_dir = (slash != NULL) || name_is_dir;
                        if (is_dir && strlen(entry) < 255) {
                            strcat(entry, "/");
                        }
                        tar_add_list_entry(entries, &entry_count, entry);
                    }
                }
            }
        }

        unsigned int size = get_size(header->size);
        ptr += ((size + 511) / 512 + 1) * 512;
    }

    for (int i = 0; i < entry_count; i++) {
        terminal_writestring(entries[i]);
        terminal_writestring(" ");
    }
    terminal_writestring("\n");
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

    while (ptr[0] != '\0') {
        struct tar_header* header = (struct tar_header*)ptr;
        int match = 1;
        int i;
        for (i = 0; filename[i] != '\0'; i++) {
            if (header->name[i] != filename[i]) {
                match = 0;
                break;
            }
        }
        if (match) {
            if (header->name[i] == '\0' || header->name[i] == ' ') {
                return (void*)(ptr + 512);
            }
        }

        unsigned int size = get_size(header->size);
        ptr += ((size + 511) / 512 + 1) * 512;
    }
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

// Function to write data to a file in the tar archive
// Note: This is a simplified implementation that would need to be expanded
// for a full write capability, as tar archives are typically read-only
int tar_write_file(void* archive, const char* filename, const void* data, unsigned int size) {
    // For now, this is a placeholder that returns an error
    // indicating that writing to tar archives is not supported
    return -1; // Not supported
}

// Function to create a new file in the tar archive
// This is a simplified implementation for demonstration
int tar_create_file(void* archive, const char* filename, const void* data, unsigned int size) {
    // This would require:
    // 1. Finding the end of the current tar archive
    // 2. Creating a new tar header for the file
    // 3. Writing the header and data to the end of the archive
    // 4. Updating the archive structure
    
    // For now, return error as this is complex to implement
    // in a read-only memory-mapped tar archive
    return -1; // Not supported in current implementation
}

// Function to update an existing file in the tar archive
// This would require rewriting the entire archive
int tar_update_file(void* archive, const char* filename, const void* data, unsigned int size) {
    // This is even more complex as it requires:
    // 1. Finding the existing file
    // 2. Creating a new tar archive with updated content
    // 3. Replacing the old archive
    
    return -1; // Not supported in current implementation
}

// Function to append data to an existing file in the tar archive
int tar_append_file(void* archive, const char* filename, const void* data, unsigned int size) {
    // This would require:
    // 1. Finding the existing file
    // 2. Reading the current content
    // 3. Creating a new file with combined content
    // 4. Replacing the old file
    
    return -1; // Not supported in current implementation
}

// Function to delete a file from the tar archive
int tar_delete_file(void* archive, const char* filename) {
    // This would require:
    // 1. Finding the file to delete
    // 2. Creating a new tar archive without that file
    // 3. Replacing the old archive
    
    return -1; // Not supported in current implementation
}

// Function to create a directory in the tar archive
int tar_create_directory(void* archive, const char* dirname) {
    // This would require:
    // 1. Creating a new tar header with typeflag '5' (directory)
    // 2. Adding it to the end of the archive
    
    return -1; // Not supported in current implementation
}

// Enhanced function to check if a path exists (supports nested paths)
int tar_path_exists_enhanced(void* archive, const char* path) {
    if (!path || path[0] == '\0') return 0;
    
    // Normalize path
    char norm_path[256];
    if (path[0] == '/') {
        strcpy(norm_path, path + 1);
    } else {
        strcpy(norm_path, path);
    }
    
    // Remove trailing slash
    int len = strlen(norm_path);
    while (len > 0 && norm_path[len - 1] == '/') {
        norm_path[--len] = '\0';
    }
    
    if (len == 0) return 1; // Root exists
    
    unsigned char* ptr = (unsigned char*)archive;
    
    while (ptr[0] != '\0') {
        struct tar_header* header = (struct tar_header*)ptr;
        
        // Check exact match
        if (strcmp(header->name, norm_path) == 0) {
            return 1;
        }
        
        // Check if this is a parent directory
        int name_len = strlen(header->name);
        if (name_len > 0 && header->name[name_len - 1] == '/') {
            name_len--; // Remove trailing slash for comparison
        }
        
        if (name_len > 0 && strncmp(header->name, norm_path, name_len) == 0) {
            if (norm_path[name_len] == '/' || norm_path[name_len] == '\0') {
                return 1;
            }
        }
        
        unsigned int size = get_size(header->size);
        ptr += ((size + 511) / 512 + 1) * 512;
    }
    
    return 0;
}

// Function to get file information (size, type, permissions)
int tar_get_file_info(void* archive, const char* filename, unsigned int* size, char* type, char* permissions) {
    unsigned char* ptr = (unsigned char*)archive;
    
    while (ptr[0] != '\0') {
        struct tar_header* header = (struct tar_header*)ptr;
        
        if (strcmp(header->name, filename) == 0) {
            if (size) *size = get_size(header->size);
            if (type) {
                switch (header->typeflag) {
                    case '0': case '\0': strcpy(type, "regular"); break;
                    case '1': strcpy(type, "hardlink"); break;
                    case '2': strcpy(type, "symlink"); break;
                    case '3': strcpy(type, "character"); break;
                    case '4': strcpy(type, "block"); break;
                    case '5': strcpy(type, "directory"); break;
                    case '6': strcpy(type, "fifo"); break;
                    default: strcpy(type, "unknown"); break;
                }
            }
            if (permissions) {
                strncpy(permissions, header->mode, 8);
                permissions[8] = '\0';
            }
            return 1; // Found
        }
        
        unsigned int file_size = get_size(header->size);
        ptr += ((file_size + 511) / 512 + 1) * 512;
    }
    
    return 0; // Not found
}
// Функция для записи данных в файл в tar-архиве
// Примечание. Это упрощенная реализация, которую необходимо расширить.
// для полной записи, поскольку tar-архивы обычно доступны только для чтения
int tar_write_file(void* archive, const char* filename, const void* data, unsigned int size) {
   // На данный момент это заполнитель, который возвращает ошибку
    // указываем, что запись в tar-архивы не поддерживается
    return -1; // Not supported
}
