#include <stdint.h>
#include <stddef.h>

// Твоя структура заголовка
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

// Твоя функция get_size
unsigned int get_size(const char *in) {
    unsigned int size = 0;
    unsigned int j;
    unsigned int count = 1;
    for (j = 11; j > 0; j--, count *= 8) 
        size += (in[j - 1] - '0') * count;
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
        // Дополнительная проверка на конец строки в заголовке
        if (match && header->name[i] == '\0') {
            return (void*)(ptr + 512);
        }

        unsigned int size = get_size(header->size);
        ptr += ((size + 511) / 512 + 1) * 512;
    }
    return NULL;
}
