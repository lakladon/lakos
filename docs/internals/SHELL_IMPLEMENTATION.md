# SHELL_IMPLEMENTATION.md - Как реализован shell

Этот документ описывает внутреннее устройство командной оболочки LakOS.

## 🎯 Общее описание

Shell в LakOS - это **интерактивная командная оболочка** с расширенными возможностями:

- **История команд** - навигация по ранее введенным командам
- **Автодополнение** - Tab-дополнение команд и путей
- **Выполнение ELF-программ** - запуск пользовательских программ
- **Перенаправление ввода/вывода** - базовая поддержка перенаправления
- **Цветной вывод** - поддержка ANSI-цветов

## 🏗️ Архитектура shell

### Основные компоненты

```
┌─────────────────────────────────────────┐
│              Пользовательский           │
│              ввод                       │
├─────────────────────────────────────────┤
│              Обработка                  │
│              ввода                      │
├─────────────────────────────────────────┤
│              Парсинг                    │
│              команд                     │
├─────────────────────────────────────────┤
│              Выполнение                 │
│              команд                     │
├─────────────────────────────────────────┤
│              Вывод                     │
│              результата                 │
└─────────────────────────────────────────┘
```

### Структура данных

#### Буфер ввода
```c
static char shell_buf[256];  // Буфер для текущей команды
static int shell_ptr = 0;    // Текущая позиция в буфере
```

#### История команд
```c
#define HISTORY_SIZE 10
static char command_history[HISTORY_SIZE][256];  // Массив истории
static int history_count = 0;                    // Количество команд в истории
static int history_index = 0;                    // Текущая позиция в истории
```

#### Список доступных команд
```c
static const char* available_commands[] = {
    "help", "cls", "ver", "pwd", "ls", "cd", "echo", "uname", "date", 
    "cat", "mkdir", "disks", "read_sector", "write_sector", "mount",
    "useradd", "passwd", "login", "userdel", "crypt", "whoami", 
    "touch", "rm", "cp", "shutdown", "reboot", "gui", "hello", "test", 
    "editor", "calc", "color"
};
static int commands_count = 32;
```

## 🔧 Реализация компонентов

### 1. Обработка ввода

#### Клавиатурный драйвер
```c
// Карта скан-кодов
unsigned char kbd_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0
};

// Карта с Shift
unsigned char kbd_map_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0
};
```

#### Обработка клавиш
```c
void shell_handle_key(char c) {
    if (c == '\n') {
        // Обработка Enter
        terminal_putchar('\n');
        shell_buf[shell_ptr] = '\0';
        
        if (shell_ptr > 0) {
            // Добавление в историю
            add_to_history(shell_buf);
            
            // Выполнение команды
            kernel_execute_command(shell_buf);
        }

        // Сброс буфера
        terminal_writestring("LakOS>");
        if (strcmp(current_user, "root") == 0) {
            terminal_writestring("\033[31mroot\033[0m");
        } else {
            terminal_writestring("\033[32m");
            terminal_writestring(current_user);
            terminal_writestring("\033[0m");
        }
        terminal_writestring(" ");
        terminal_writestring(current_dir);
        terminal_writestring(" \033[36m(uid:");
        char buf[16];
        itoa(get_current_uid(), buf);
        terminal_writestring(buf);
        terminal_writestring(",gid:");
        itoa(get_current_gid(), buf);
        terminal_writestring(buf);
        terminal_writestring(")\033[0m ");
        shell_ptr = 0;
    } 
    else if (c == '\b') {
        // Обработка Backspace
        if (shell_ptr > 0) {
            shell_ptr--;
            terminal_putchar('\b');
            terminal_putchar(' ');
            terminal_putchar('\b');
        }
    } 
    else if (shell_ptr < 255) {
        // Обработка обычных символов
        shell_buf[shell_ptr++] = c;
        terminal_putchar(c);
    }
}
```

### 2. История команд

#### Добавление команды в историю
```c
void add_to_history(const char* command) {
    if (strlen(command) > 0) {
        // Добавление в историю
        strcpy(command_history[history_count % HISTORY_SIZE], command);
        history_count++;
        history_index = history_count; // Указываем на новую команду
    }
}
```

#### Получение предыдущей команды
```c
void get_previous_history() {
    if (history_count > 0) {
        int prev_index = (history_index - 1 + HISTORY_SIZE) % HISTORY_SIZE;
        if (history_index > 0) {
            // Очистка текущей строки
            for (int i = 0; i < shell_ptr; i++) {
                terminal_putchar('\b');
                terminal_putchar(' ');
                terminal_putchar('\b');
            }
            
            // Загрузка предыдущей команды
            strcpy(shell_buf, command_history[prev_index]);
            shell_ptr = strlen(shell_buf);
            
            // Отображение команды
            terminal_writestring(shell_buf);
        }
    }
}
```

#### Получение следующей команды
```c
void get_next_history() {
    if (history_count > 0 && history_index < history_count) {
        history_index++;
        if (history_index >= history_count) {
            // Очистка текущей строки
            for (int i = 0; i < shell_ptr; i++) {
                terminal_putchar('\b');
                terminal_putchar(' ');
                terminal_putchar('\b');
            }
            shell_ptr = 0;
            shell_buf[0] = '\0';
        } else {
            int next_index = history_index % HISTORY_SIZE;
            // Очистка текущей строки
            for (int i = 0; i < shell_ptr; i++) {
                terminal_putchar('\b');
                terminal_putchar(' ');
                terminal_putchar('\b');
            }
            
            // Загрузка следующей команды
            strcpy(shell_buf, command_history[next_index]);
            shell_ptr = strlen(shell_buf);
            
            // Отображение команды
            terminal_writestring(shell_buf);
        }
    }
}
```

### 3. Автодополнение

#### Поиск совпадающих команд
```c
int find_command_matches(const char* prefix, char matches[][32]) {
    int match_count = 0;
    int prefix_len = strlen(prefix);
    
    for (int i = 0; i < commands_count; i++) {
        if (strncmp(available_commands[i], prefix, prefix_len) == 0) {
            strcpy(matches[match_count], available_commands[i]);
            match_count++;
        }
    }
    return match_count;
}
```

#### Поиск совпадающих директорий
```c
int find_directory_matches(const char* prefix, char matches[][32]) {
    int match_count = 0;
    int prefix_len = strlen(prefix);
    
    // Простая реализация для встроенных директорий
    if (strcmp(prefix, "") == 0) {
        strcpy(matches[match_count++], "bin/");
        strcpy(matches[match_count++], "home/");
    } else if (strncmp(prefix, "bin", prefix_len) == 0) {
        strcpy(matches[match_count++], "bin/");
    } else if (strncmp(prefix, "home", prefix_len) == 0) {
        strcpy(matches[match_count++], "home/");
    }
    return match_count;
}
```

#### Выполнение автодополнения
```c
void perform_completion() {
    if (shell_ptr == 0) return;
    
    // Поиск текущего слова
    int start = 0;
    for (int i = shell_ptr - 1; i >= 0; i--) {
        if (shell_buf[i] == ' ') {
            start = i + 1;
            break;
        }
    }
    
    char current_word[32];
    int word_len = shell_ptr - start;
    if (word_len >= 32) word_len = 31;
    for (int i = 0; i < word_len; i++) {
        current_word[i] = shell_buf[start + i];
    }
    current_word[word_len] = '\0';
    
    // Проверка, является ли это командой cd
    int is_cd_command = 0;
    if (start == 0) {
        char first_word[32];
        int space_pos = -1;
        for (int i = 0; i < shell_ptr; i++) {
            if (shell_buf[i] == ' ') {
                space_pos = i;
                break;
            }
        }
        if (space_pos > 0) {
            int len = space_pos;
            if (len >= 32) len = 31;
            for (int i = 0; i < len; i++) {
                first_word[i] = shell_buf[i];
            }
            first_word[len] = '\0';
            if (strcmp(first_word, "cd") == 0) {
                is_cd_command = 1;
            }
        }
    }
    
    // Поиск совпадений
    char matches[10][32];
    int match_count = 0;
    
    if (is_cd_command) {
        match_count = find_directory_matches(current_word, matches);
    } else {
        match_count = find_command_matches(current_word, matches);
    }
    
    if (match_count == 1) {
        // Единственное совпадение - дополнение
        for (int i = 0; i < word_len; i++) {
            terminal_putchar('\b');
            terminal_putchar(' ');
            terminal_putchar('\b');
        }
        
        strcpy(shell_buf + start, matches[0]);
        shell_ptr = start + strlen(matches[0]);
        terminal_writestring(matches[0]);
    } else if (match_count > 1) {
        // Несколько совпадений - показать варианты
        terminal_putchar('\n');
        for (int i = 0; i < match_count; i++) {
            terminal_writestring(matches[i]);
            terminal_writestring(" ");
        }
        terminal_putchar('\n');
        
        // Восстановление приглашения и текущей команды
        terminal_writestring("LakOS>");
        if (strcmp(current_user, "root") == 0) {
            terminal_writestring("\033[31mroot\033[0m");
        } else {
            terminal_writestring("\033[32m");
            terminal_writestring(current_user);
            terminal_writestring("\033[0m");
        }
        terminal_writestring(" ");
        terminal_writestring(current_dir);
        terminal_writestring(" \033[36m(uid:");
        char buf[16];
        itoa(get_current_uid(), buf);
        terminal_writestring(buf);
        terminal_writestring(",gid:");
        itoa(get_current_gid(), buf);
        terminal_writestring(buf);
        terminal_writestring(")\033[0m ");
        terminal_writestring(shell_buf);
    }
}
```

### 4. Выполнение команд

#### Парсинг команды
```c
void kernel_execute_command(const char* input) {
    // Пропуск начальных пробелов
    while (*input == ' ') input++;
    if (*input == '\0') return;

    // Разделение на команду и аргументы
    char cmd[64];
    int i = 0;
    while (input[i] && input[i] != ' ' && i < 63) {
        cmd[i] = input[i];
        i++;
    }
    cmd[i] = '\0';
    
    const char* args = input + i;
    while (*args == ' ') args++;

    // Выполнение команды
    if (strcmp(cmd, "help") == 0) {
        terminal_writestring("Lakos OS Commands: help, cls, ver, pwd, ls, cd, echo, uname, date, cat, mkdir, disks, read_sector, write_sector, mount, useradd, passwd, login, userdel, crypt, whoami, touch, rm, cp, shutdown, reboot, gui\n");
        terminal_writestring("Available programs: hello, test, editor, calc\n");
    }
    else if (strcmp(cmd, "cls") == 0) {
        terminal_initialize();
    }
    // ... другие команды
    else {
        // Проверка на пользовательскую программу
        if (is_file_in_path(cmd, pathbin)) {
            execute_binary(cmd);
        } else {
            terminal_writestring("Error: command '");
            terminal_writestring(cmd);
            terminal_writestring("' not found.\n");
        }
    }
}
```

#### Выполнение ELF-программ
```c
static void execute_binary(const char* name) {
    if (!tar_archive) {
        terminal_writestring("No tar archive loaded.\n");
        return;
    }
    
    // Формирование пути к программе
    char path[256];
    strcpy(path, "bin/");
    strcpy(path + 4, name);

    // Поиск программы в архиве
    void* data = tar_lookup(tar_archive, path);
    if (!data) {
        terminal_writestring("Binary not found in tar.\n");
        return;
    }

    // Проверка ELF заголовка
    Elf32_Ehdr* ehdr = (Elf32_Ehdr*)data;
    if (ehdr->e_ident[0] != 0x7F || ehdr->e_ident[1] != 'E' || 
        ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F') {
        terminal_writestring("Not a valid ELF file.\n");
        return;
    }
    
    if (ehdr->e_machine != 3) { // i386
        terminal_writestring("Unsupported ELF architecture.\n");
        return;
    }
    
    if (ehdr->e_entry < 0x100000) {
        terminal_writestring("Invalid entry point address.\n");
        return;
    }

    // Загрузка сегментов
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
            
            // Копирование данных
            memcpy((void*)phdr[i].p_vaddr, (uint8_t*)data + phdr[i].p_offset, phdr[i].p_filesz);
            
            // Обнуление BSS
            memset((void*)(phdr[i].p_vaddr + phdr[i].p_filesz), 0, phdr[i].p_memsz - phdr[i].p_filesz);
        }
    }

    // Подготовка стека
    uint32_t* stack = (uint32_t*)0x200000;
    uint32_t* sp = &stack[255];
    *(--sp) = 0; // argv[1] = NULL
    *(--sp) = (uint32_t)name; // argv[0]
    *(--sp) = 1; // argc

    // Вызов программы
    typedef int (*entry_t)(int, char**);
    entry_t entry = (entry_t)ehdr->e_entry;
    int ret = entry(1, (char**)sp);

    terminal_writestring("Binary returned: ");
    terminal_putchar('0' + ret);
    terminal_writestring("\n");
}
```

## 🎨 Интерфейс пользователя

### Цветное приглашение
```c
// Цветное приглашение shell
void print_prompt(void) {
    terminal_writestring("LakOS>");
    
    // Цвет в зависимости от пользователя
    if (strcmp(current_user, "root") == 0) {
        terminal_writestring("\033[31mroot\033[0m"); // Красный для root
    } else {
        terminal_writestring("\033[32m");
        terminal_writestring(current_user);
        terminal_writestring("\033[0m"); // Зеленый для обычных пользователей
    }
    
    terminal_writestring(" ");
    terminal_writestring(current_dir);
    
    // UID и GID
    terminal_writestring(" \033[36m(uid:");
    char buf[16];
    itoa(get_current_uid(), buf);
    terminal_writestring(buf);
    terminal_writestring(",gid:");
    itoa(get_current_gid(), buf);
    terminal_writestring(buf);
    terminal_writestring(")\033[0m ");
}
```

### Обработка специальных символов
```c
// Обработка ANSI escape-последовательностей
void terminal_writestring(const char* s) {
    while (*s) {
        if (*s == '\033' && *(s+1) == '[') {
            s += 2;
            int code = 0;
            while (*s >= '0' && *s <= '9') {
                code = code * 10 + (*s - '0');
                s++;
            }
            if (*s == 'm') {
                s++;
                if (code == 0) current_attr = 0x0F; // reset to white
                else if (code == 31) current_attr = 0x04; // red fg
                else if (code == 32) current_attr = 0x02; // green fg
                else if (code == 33) current_attr = 0x0E; // yellow fg
                else if (code == 36) current_attr = 0x03; // cyan fg
            } else {
                // invalid, skip
                while (*s && *s != 'm') s++;
                if (*s == 'm') s++;
            }
        } else {
            terminal_putchar(*s);
            s++;
        }
    }
}
```

## 🔄 Главный цикл shell

### Инициализация shell
```c
void shell_main() {
    terminal_writestring("Shell start\n");
    terminal_writestring("lkaS  ");
    terminal_writestring(SHELL_VERSION);
    terminal_writestring("\n");
    
    // Инициализация пользователей
    init_users();
    
    // Авторизация
    char username[32], password[32];
    while (1) {
        terminal_writestring("Login: ");
        read_line(username, 31, 1);
        terminal_writestring("Password: ");
        read_line(password, 31, 0);
        terminal_putchar('\n');
        
        // Аутентификация
        if (authenticate_user(username, password)) {
            terminal_writestring("Login successful for user: ");
            terminal_writestring(current_user);
            terminal_writestring("\n");
            break;
        } else {
            terminal_writestring("Invalid login\n");
        }
    }

    terminal_writestring("Login successful\n");
    
    // Главный цикл
    while(1) {
        // Обработка прерываний клавиатуры
        if (inb(0x64) & 0x1) {
            uint8_t scancode = inb(0x60);
            
            // Обработка специальных клавиш
            if ((scancode & 0x7F) == 42 || (scancode & 0x7F) == 54) {
                shift_pressed = !(scancode & 0x80);
            } else if (scancode == 58) {
                if (!(scancode & 0x80)) caps_locked = !caps_locked;
            } else if (!(scancode & 0x80)) {
                if (scancode == KEY_UP) {
                    get_previous_history();
                } else if (scancode == KEY_DOWN) {
                    get_next_history();
                } else if (scancode == KEY_TAB) {
                    perform_completion();
                } else {
                    // Обычные символы
                    int is_letter = (scancode >= 16 && scancode <= 25) || 
                                   (scancode >= 30 && scancode <= 38) || 
                                   (scancode >= 44 && scancode <= 50);
                    int uppercase = shift_pressed || (caps_locked && is_letter);
                    char c = uppercase ? kbd_map_shift[scancode] : kbd_map[scancode];
                    if (c != 0) shell_handle_key(c);
                }
            }
        }
    }
}
```

## 📊 Производительность

### Преимущества реализации
1. **Минимальное потребление памяти** - небольшие буферы
2. **Быстрая реакция** - прямая обработка прерываний
3. **Простота** - минимальная логика парсинга
4. **Надежность** - обработка ошибок и граничных случаев

### Ограничения
1. **Ограниченный размер команд** - 256 символов
2. **Простой парсинг** - нет сложных конструкций
3. **Нет фоновых процессов** - только последовательное выполнение
4. **Нет pipe и каналов** - только базовое перенаправление

## 🚀 Возможные улучшения

### 1. Расширенный парсинг
- Поддержка pipe (|)
- Поддержка каналов
- Сложные конструкции (&&, ||)
- Переменные окружения

### 2. Улучшение интерфейса
- Поддержка мыши
- Графический интерфейс
- Автодополнение с подсказками
- История с поиском

### 3. Функциональность
- Фоновые процессы
- Сигналы
- Джобы
- Алиасы команд

### 4. Безопасность
- Проверка прав доступа
- Ограничение ресурсов
- Изоляция процессов

---

**Shell LakOS** - это простая, но функциональная командная оболочка, обеспечивающая базовые возможности для взаимодействия с операционной системой.