#include <stdint.h>
#include <stddef.h>

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
unsigned int get_size(const char *in) {
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
            // Например: kprint(header->name); kprint("\n");
        }

        unsigned int size = get_size(header->size);
        ptr += ((size + 511) / 512 + 1) * 512;
    }
}

// Твоя функция поиска
void* tar_lookup(void* archive, const char* filename) {
    unsigned char* ptr = (unsigned char*)archive;
    terminal_writestring("Tar lookup for: ");
    terminal_writestring(filename);
    terminal_writestring("\n");

    while (ptr[0] != '\0') {
        struct tar_header* header = (struct tar_header*)ptr;
        terminal_writestring("Header name: ");
        terminal_writestring(header->name);
        terminal_writestring("\n");

        int match = 1;
        int i;
        for (i = 0; filename[i] != '\0'; i++) {
            if (header->name[i] != filename[i]) {
                match = 0;
                break;
            }
        }
        // Дополнительная проверка на конец строки в заголовке
        if (match && header->name[i] == '\0') {
            terminal_writestring("Match found!\n");
            return (void*)(ptr + 512);
        }

        unsigned int size = get_size(header->size);
        terminal_writestring("Size: ");
        // Need to print size, but no itoa in tar.c
        // Skip for now
        ptr += ((size + 511) / 512 + 1) * 512;
    }
    terminal_writestring("No match found.\n");
    return NULL;
}
