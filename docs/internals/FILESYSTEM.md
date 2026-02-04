# FILESYSTEM.md - Tar-FS устройство

Этот документ описывает реализацию файловой системы на основе tar-архивов в LakOS.

## 📁 Общее описание

LakOS использует **Tar-FS** - файловую систему, основанную на чтении tar-архивов как файловой системы. Это простое, но эффективное решение для учебной операционной системы.

### Особенности Tar-FS:
- **Только для чтения** - файлы нельзя изменять или удалять
- **Вшитый в образ** - архив modules.tar встроен в загрузочный образ
- **Простая реализация** - минимальный код для максимальной надежности
- **Поддержка каталогов** - полная иерархия каталогов
- **Поиск по имени** - быстрый поиск файлов по полному пути

## 🏗️ Архитектура файловой системы

### Структура tar-архива

Tar-архив состоит из последовательности записей:

```
┌─────────────────┐
│   Заголовок    │  512 байт
├─────────────────┤
│   Данные       │  Переменный размер
│   (выравнено   │
│   до 512 байт) │
├─────────────────┤
│   Заголовок    │  512 байт
├─────────────────┤
│   Данные       │
│   ...          │
└─────────────────┘
```

### Заголовок tar-записи

```c
struct tar_header {
    char name[100];     // Имя файла
    char mode[8];       // Права доступа
    char uid[8];        // UID владельца
    char gid[8];        // GID владельца
    char size[12];      // Размер файла в байтах (восьмеричное)
    char mtime[12];     // Время модификации (восьмеричное)
    char checksum[8];   // Контрольная сумма
    char typeflag;      // Тип файла
    char linkname[100]; // Имя ссылки
    char magic[6];      // Магическое число "ustar"
} __attribute__((packed));
```

### Типы файлов
- `'0'` или `'\0'` - Обычный файл
- `'1'` - Символическая ссылка
- `'2'` - Символьное устройство
- `'3'` - Блочное устройство
- `'4'` - Каталог
- `'5'` - Каталог (устаревший формат)
- `'D'` - Каталог (устаревший формат)

## 🔧 Реализация

### Основные функции

#### 1. Поиск файла
```c
void* tar_lookup(void* archive, const char* filename) {
    unsigned char* ptr = (unsigned char*)archive;

    while (ptr[0] != '\0') {
        struct tar_header* header = (struct tar_header*)ptr;
        
        // Сравнение имен
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
                return (void*)(ptr + 512); // Возвращаем указатель на данные
            }
        }

        // Переход к следующей записи
        unsigned int size = get_size(header->size);
        ptr += ((size + 511) / 512 + 1) * 512;
    }
    return NULL; // Файл не найден
}
```

#### 2. Получение размера файла
```c
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

    return -1; // Файл не найден
}
```

#### 3. Проверка существования пути
```c
int tar_check_path_exists(void* archive, const char* path) {
    if (!path) return 0;

    // Нормализация пути
    const char* norm = path;
    if (norm[0] == '/') norm++;
    
    char path_buf[256];
    int path_len = strlen(norm);
    while (path_len > 0 && norm[path_len - 1] == '/') path_len--;
    if (path_len >= 255) path_len = 255;
    strncpy(path_buf, norm, path_len);
    path_buf[path_len] = '\0';

    if (path_buf[0] == '\0') return 1; // root всегда существует

    unsigned char* ptr = (unsigned char*)archive;
    
    while (ptr[0] != '\0') {
        struct tar_header* header = (struct tar_header*)ptr;
        
        // Проверка точного совпадения
        if (strcmp(header->name, path_buf) == 0) {
            return 1;
        }

        // Проверка директории с завершающим '/'
        int name_len = strlen(header->name);
        if (name_len > 0 && header->name[name_len - 1] == '/' &&
            name_len - 1 == path_len &&
            strncmp(header->name, path_buf, path_len) == 0) {
            return 1;
        }
        
        // Проверка родительской директории
        if (name_len < path_len && 
            strncmp(header->name, path_buf, name_len) == 0 &&
            path_buf[name_len] == '/') {
            return 1;
        }
        
        unsigned int size = get_size(header->size);
        ptr += ((size + 511) / 512 + 1) * 512;
    }
    return 0; // Путь не найден
}
```

#### 4. Получение списка каталогов
```c
void tar_get_directories(void* archive, char directories[][256], int* count) {
    unsigned char* ptr = (unsigned char*)archive;
    *count = 0;

    while (ptr[0] != '\0') {
        struct tar_header* header = (struct tar_header*)ptr;

        if (header->name[0] != '\0') {
            int name_len = strlen(header->name);
            
            // Обработка каталогов
            if (header->typeflag == '4' || header->typeflag == '5' || header->typeflag == 'D') {
                tar_add_parent_directories(directories, count, header->name, name_len);
            } else {
                // Обработка файлов - добавление родительских каталогов
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
```

#### 5. Листинг каталога
```c
void tar_list_directory(void* archive, const char* dirpath) {
    unsigned char* ptr = (unsigned char*)archive;
    if (!ptr) return;

    // Нормализация пути
    const char* norm = dirpath ? dirpath : "";
    if (norm[0] == '/') norm++;
    char base[256];
    int base_len = strlen(norm);
    while (base_len > 0 && norm[base_len - 1] == '/') base_len--;
    if (base_len >= 255) base_len = 255;
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
            int name_is_dir = (header->typeflag == '4' || header->typeflag == '5' || header->typeflag == 'D' ||
                               (name_len > 0 && name[name_len - 1] == '/'));

            if (prefix_len == 0) {
                // Корневой каталог
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
                // Подкаталог
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

    // Вывод результатов
    for (int i = 0; i < entry_count; i++) {
        terminal_writestring(entries[i]);
        terminal_writestring(" ");
    }
    terminal_writestring("\n");
}
```

## 📊 Производительность

### Преимущества Tar-FS
1. **Простота реализации** - минимальный код
2. **Надежность** - проверенный формат
3. **Компактность** - эффективное хранение
4. **Портабельность** - стандартный формат

### Ограничения Tar-FS
1. **Только для чтения** - нельзя изменять файлы
2. **Линейный поиск** - O(n) сложность поиска
3. **Нет метаданных** - ограниченная информация о файлах
4. **Нет прав доступа** - все файлы доступны всем

### Оптимизации
1. **Кэширование** - хранение часто используемых файлов
2. **Индексация** - создание таблицы соответствий
3. **Параллельный доступ** - многопоточное чтение

## 🔍 Интеграция с ядром

### Встраивание архива
```c
// В Makefile
modules.o: modules.tar
	$(OBJCOPY) -I binary -O elf32-i386 -B i386 $< $@

// В kernel/kernel.c
extern char _binary_modules_tar_start[];
void* tar_archive = (void*)&_binary_modules_tar_start;
```

### Использование в командах
```c
// В kernel/commands.c
void kernel_execute_command(const char* input) {
    // ...
    else if (strcmp(cmd, "cat") == 0) {
        const char* filename = args;
        if (strlen(filename) == 0) {
            terminal_writestring("cat: missing file name\n");
        } else {
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
                    for (int idx = 0; idx < size; idx++) {
                        terminal_putchar(bytes[idx]);
                    }
                    terminal_putchar('\n');
                } else {
                    terminal_writestring("cat: ");
                    terminal_writestring(filename);
                    terminal_writestring(": No such file\n");
                }
            }
        }
    }
    // ...
}
```

## 🚀 Возможные улучшения

### 1. Поддержка записи
- Реализация временной файловой системы
- Поддержка виртуальных файлов
- Кэширование изменений

### 2. Оптимизация производительности
- Создание индекса файлов
- Кэширование часто используемых файлов
- Параллельный доступ к данным

### 3. Расширенные возможности
- Поддержка симлинков
- Расширенные метаданные
- Права доступа
- Сжатие данных

### 4. Новые форматы
- Поддержка других архивных форматов
- Интеграция с реальными файловыми системами
- Поддержка сетевых файловых систем

## 📚 Примеры использования

### Создание tar-архива
```bash
# Создание архива для LakOS
cd rootfs
tar -cf ../modules.tar .

# Проверка содержимого
tar -tf modules.tar
```

### Интеграция в сборку
```makefile
# В Makefile
modules.tar: rootfs/
	cd rootfs && find . -type f | tar -cf ../$@ -T -

modules.o: modules.tar
	$(OBJCOPY) -I binary -O elf32-i386 -B i386 $< $@

# Добавление в список объектов
KERNEL_OBJS = ... modules.o
```

### Использование в коде
```c
// Пример чтения файла из tar-архива
void read_config_file(void) {
    if (tar_archive) {
        void* data = tar_lookup(tar_archive, "etc/config.txt");
        if (data) {
            char* content = (char*)data;
            // Обработка конфигурации
            parse_config(content);
        }
    }
}
```

---

**Tar-FS** - это простое и надежное решение для файловой системы в учебной операционной системе, обеспечивающее базовые функции для хранения и доступа к файлам.