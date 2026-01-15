#include <stdint.h>
#include <stddef.h>

extern void terminal_writestring(const char* s);

void itoa(int n, char* buf) {
    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    int i = 0;
    while (n > 0) {
        buf[i++] = '0' + n % 10;
        n /= 10;
    }
    buf[i] = '\0';
    // reverse
    for (int j = 0; j < i/2; j++) {
        char t = buf[j];
        buf[j] = buf[i-1-j];
        buf[i-1-j] = t;
    }
}

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
            terminal_writestring(header->name);
            terminal_writestring("\n");
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
                char buf[16];
                itoa(i, buf);
                terminal_writestring(buf);
                terminal_writestring(": header '");
                char cbuf[2] = {header->name[i], '\0'};
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
