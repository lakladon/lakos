#include <stdint.h>
#include <stddef.h>

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

unsigned int get_size(const char *in) {
    unsigned int size = 0;
    unsigned int j;
    unsigned int count = 1;
    for (j = 11; j > 0; j--, count *= 8) 
        size += (in[j - 1] - '0') * count;
    return size;
}

void* tar_lookup(void* archive, const char* filename) {
    unsigned char* ptr = (unsigned char*)archive;

    while (ptr[0] != '\0') {
        struct tar_header* header = (struct tar_header*)ptr; // Добавлена *

        // Сравнение имен файлов
        int match = 1;
        for (int i = 0; filename[i] != '\0'; i++) {
            if (header->name[i] != filename[i]) {
                match = 0;
                break;
            }
        }
        
        // Если нашли файл — возвращаем адрес сразу за заголовком (512 байт)
        if (match && (header->name[0] != '\0')) {
            return (void*)(ptr + 512);
        }

        // Вычисляем размер файла и переходим к следующему заголовку
        unsigned int size = get_size(header->size);
        // TAR выравнивает файлы по 512 байт. 
        // Формула: заголовок (512) + данные (округленные вверх до 512)
        ptr += ((size + 511) / 512 + 1) * 512;
    }

    return NULL; // Файл не найден
}
// this is the new shit by lakladon
