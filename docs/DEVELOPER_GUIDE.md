# DEVELOPER_GUIDE.md - Руководство разработчика

Это руководство предназначено для разработчиков, желающих внести вклад в развитие LakOS или создать свои собственные модули для системы.

## 🎯 Начало работы

### Требования к разработке

Для разработки под LakOS вам понадобятся:

- **GCC** с поддержкой i386
- **NASM** - ассемблер
- **QEMU** - для тестирования
- **GDB** - для отладки
- **Знание C** на среднем уровне
- **Базовые знания ассемблера** (x86)

### Настройка среды разработки

#### Linux (Ubuntu/Debian)
```bash
sudo apt install gcc gcc-multilib nasm qemu-system-i386 gdb
```

#### macOS
```bash
brew install gcc nasm qemu
```

#### Windows (WSL2)
```bash
sudo apt install gcc gcc-multilib nasm qemu-system-i386 gdb
```

## 🏗️ Структура кода

### Ядро (kernel/)

```
kernel/
├── kernel.c          # Главная функция ядра
├── shell.c           # Командная оболочка
├── commands.c        # Встроенные команды
├── lib.c             # Библиотечные функции
├── vga.c             # VGA драйвер
├── gui.c             # Графический интерфейс
├── users.c           # Система пользователей
├── crypt.c           # Криптографические функции
├── gdt.c             # Global Descriptor Table
├── idt.c             # Interrupt Descriptor Table
├── isr.c             # Обработчики прерываний
├── start.asm         # Точка входа
├── drivers/          # Драйверы устройств
│   ├── ata.c         # ATA драйвер
│   ├── mouse.c       # Мышь
│   └── io.h          # Ввод/вывод
├── fs/               # Файловая система
│   └── tar.c         # Tar-FS
└── include/          # Заголовочные файлы
    ├── *.h           # Интерфейсы
    └── version.h     # Версия
```

### Bootloader (boot/)

```
boot/
└── boot.asm          # Загрузчик Multiboot
```

### RootFS (rootfs/)

```
rootfs/
├── bin/              # Пользовательские программы
├── etc/              # Конфигурация
├── home/             # Домашние директории
└── test.txt          # Тестовые файлы
```

## 📝 Написание кода

### Стиль кодирования

#### C код
```c
// Используйте snake_case для функций и переменных
void my_function_name(void) {
    int local_variable = 0;
    
    // Отступы - 4 пробела
    if (condition) {
        do_something();
    }
}

// Используйте PascalCase для структур
typedef struct {
    int field1;
    char field2[32];
} MyStruct;

// Макросы - UPPER_CASE
#define MAX_BUFFER_SIZE 256
#define DEBUG_PRINT(msg) printf("DEBUG: %s\n", msg)
```

#### Ассемблер
```asm
; Используйте понятные метки
section .text
global my_function

my_function:
    push ebp
    mov ebp, esp
    
    ; Комментируйте сложные операции
    mov eax, [ebp + 8]  ; Загружаем первый аргумент
    
    leave
    ret
```

### Безопасность

#### Проверка границ
```c
// Всегда проверяйте границы массивов
void safe_copy(char* dest, const char* src, size_t max_len) {
    if (!dest || !src || max_len == 0) {
        return;
    }
    
    size_t i = 0;
    while (i < max_len - 1 && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}
```

#### Проверка NULL
```c
// Всегда проверяйте указатели на NULL
void process_data(char* data) {
    if (data == NULL) {
        return;
    }
    
    // Работаем с данными
    printf("%s\n", data);
}
```

#### Защита от переполнения
```c
// Используйте безопасные функции
void safe_string_copy(char* dest, const char* src, size_t size) {
    if (size > 0) {
        strncpy(dest, src, size - 1);
        dest[size - 1] = '\0';
    }
}
```

## 🔧 Разработка драйверов

### Создание нового драйвера

#### Шаг 1: Создайте файл драйвера
```c
// kernel/drivers/my_driver.c
#include "include/lib.h"

// Определите порты ввода/вывода
#define MY_DEVICE_PORT 0x123

// Функция инициализации
void my_driver_init(void) {
    // Инициализация устройства
    outb(MY_DEVICE_PORT, 0x01);
}

// Функция чтения
uint8_t my_driver_read(void) {
    return inb(MY_DEVICE_PORT);
}

// Функция записи
void my_driver_write(uint8_t data) {
    outb(MY_DEVICE_PORT, data);
}
```

#### Шаг 2: Добавьте заголовок
```c
// kernel/include/my_driver.h
#ifndef MY_DRIVER_H
#define MY_DRIVER_H

void my_driver_init(void);
uint8_t my_driver_read(void);
void my_driver_write(uint8_t data);

#endif
```

#### Шаг 3: Интегрируйте в ядро
```c
// kernel/kernel.c
#include "include/my_driver.h"

void kernel_main(void) {
    // Инициализация других компонентов...
    
    // Инициализация вашего драйвера
    my_driver_init();
    
    // Запуск shell...
}
```

#### Шаг 4: Добавьте в Makefile
```makefile
# В список объектных файлов
KERNEL_OBJS = ... kernel/drivers/my_driver.o

# В правила сборки
kernel/drivers/my_driver.o: kernel/drivers/my_driver.c
	$(CC) $(CFLAGS) -c $< -o $@
```

### Пример: Простой драйвер клавиатуры

```c
// kernel/drivers/keyboard.c
#include "include/lib.h"
#include "include/io.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

// Буфер ввода
static char keyboard_buffer[256];
static int buffer_pos = 0;

// Обработчик прерывания клавиатуры
void keyboard_handler(void) {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    // Простая карта клавиш
    static const char keymap[128] = {
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
        '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[',
        ']', '\n', 0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
        '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.',
        '/', 0, '*', 0, ' '
    };
    
    if (scancode < 128) {
        char c = keymap[scancode];
        if (c != 0 && buffer_pos < 255) {
            keyboard_buffer[buffer_pos++] = c;
        }
    }
}

// Инициализация драйвера
void keyboard_init(void) {
    // Регистрация обработчика прерывания
    register_interrupt_handler(1, keyboard_handler);
}
```

## 📦 Создание пользовательских программ

### ELF формат

LakOS поддерживает загрузку ELF-программ. Программы должны:

1. Быть скомпилированы для i386
2. Иметь правильный entry point
3. Использовать системные вызовы для ввода/вывода

### Пример простой программы

```c
// rootfs/bin/hello.c
#include <stdint.h>

// Системные вызовы
#define SYS_WRITE 4
#define SYS_EXIT 1

// Функция для системных вызовов
static int syscall(int num, int arg1, int arg2, int arg3) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3)
    );
    return ret;
}

// Функция вывода строки
void print(const char* str) {
    syscall(SYS_WRITE, 1, (int)str, strlen(str));
}

// Точка входа
int main(void) {
    print("Hello from user program!\n");
    return 0;
}
```

### Компиляция пользовательской программы

```makefile
# В Makefile добавьте правило
rootfs/bin/hello: rootfs/bin/hello.c
	$(CC) -m32 -ffreestanding -O0 -fno-pie -no-pie -c $< -o /tmp/hello.o
	$(LD) -m elf_i386 -e main -Ttext 0x200000 --unresolved-symbols=ignore-all -o $@ /tmp/hello.o
```

## 🐛 Отладка

### Использование GDB

#### Настройка отладки
```bash
# Запуск QEMU с отладочным портом
qemu-system-i386 -cdrom lakos.iso -m 512M -s -S

# В другом терминале запустите GDB
gdb lakos.bin
(gdb) target remote localhost:1234
(gdb) info registers
(gdb) list
(gdb) break kernel_main
(gdb) continue
```

#### Полезные команды GDB
```gdb
# Просмотр регистров
info registers

# Просмотр памяти
x/16xw 0x100000

# Просмотр стека
info stack

# Просмотр функций
info functions

# Просмотр переменных
info variables
```

### Отладочные макросы

```c
// include/debug.h
#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) \
    do { \
        printf("[DEBUG] %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

#endif
```

### Логирование

```c
// Пример использования логирования
void my_function(void) {
    DEBUG_PRINT("Function started");
    
    // Ваш код
    
    DEBUG_PRINT("Function completed");
}
```

## 🧪 Тестирование

### Unit-тесты

#### Создание тестов
```c
// tests/test_lib.c
#include "include/lib.h"
#include <assert.h>

void test_strlen(void) {
    assert(strlen("") == 0);
    assert(strlen("hello") == 5);
    assert(strlen("hello world") == 11);
}

void test_strcmp(void) {
    assert(strcmp("", "") == 0);
    assert(strcmp("hello", "hello") == 0);
    assert(strcmp("hello", "world") < 0);
    assert(strcmp("world", "hello") > 0);
}

void run_tests(void) {
    test_strlen();
    test_strcmp();
    printf("All tests passed!\n");
}
```

#### Запуск тестов
```makefile
# В Makefile
test: lakos.bin
	qemu-system-i386 -cdrom lakos.iso -m 512M -nographic -serial stdio
```

### Интеграционное тестирование

#### Автоматические тесты
```bash
#!/bin/bash
# tests/run_tests.sh

echo "Building LakOS..."
make clean && make all

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Running tests..."
qemu-system-i386 -cdrom lakos.iso -m 512M -nographic -serial stdio << EOF
echo "Test 1: Basic commands"
help
echo "Test 2: File operations"
ls
echo "Test 3: User operations"
whoami
shutdown
EOF
```

## 📋 Системные вызовы

### Добавление нового системного вызова

#### Шаг 1: Определите номер системного вызова
```c
// include/syscalls.h
#define SYS_MY_CALL 42
```

#### Шаг 2: Реализуйте обработчик
```c
// kernel/syscalls.c
void syscall_handler(int syscall_num, int arg1, int arg2, int arg3) {
    switch (syscall_num) {
        case SYS_MY_CALL:
            my_call_handler(arg1, arg2, arg3);
            break;
        // Другие системные вызовы...
    }
}
```

#### Шаг 3: Реализуйте функцию
```c
void my_call_handler(int arg1, int arg2, int arg3) {
    // Ваша реализация
    printf("My syscall called with args: %d, %d, %d\n", arg1, arg2, arg3);
}
```

#### Шаг 4: Добавьте в IDT
```c
// kernel/idt.c
void idt_init(void) {
    // Другие обработчики...
    
    // Регистрация обработчика системных вызовов
    set_idt_entry(0x80, (uint32_t)syscall_handler, 0x08, 0x8E);
}
```

## 🔒 Безопасность

### Проверка прав доступа
```c
// Проверка прав пользователя
int check_permission(int required_uid, int required_gid) {
    if (get_current_uid() != required_uid && get_current_gid() != required_gid) {
        return -1; // Доступ запрещен
    }
    return 0; // Доступ разрешен
}
```

### Защита памяти
```c
// Проверка адреса памяти
int is_valid_address(void* addr) {
    if (addr == NULL) {
        return 0;
    }
    
    // Проверка, что адрес в пределах разрешенной памяти
    if ((uint32_t)addr < 0x100000 || (uint32_t)addr > 0x1000000) {
        return 0;
    }
    
    return 1;
}
```

## 📚 Ресурсы для разработчиков

### Документация
- [x86 Instruction Set Reference](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- [OSDev Wiki](https://wiki.osdev.org/Main_Page)
- [GRUB Manual](https://www.gnu.org/software/grub/manual/)

### Инструменты
- [QEMU](https://www.qemu.org/)
- [GDB](https://www.gnu.org/software/gdb/)
- [NASM](https://www.nasm.us/)

### Примеры кода
- [Linux Kernel](https://github.com/torvalds/linux)
- [Minix](https://github.com/Stichting-MINIX-Research-Foundation/minix)
- [FreeBSD](https://github.com/freebsd/freebsd-src)

## 🚀 Распространенные задачи

### Добавление новой команды
1. Реализуйте функцию команды в `commands.c`
2. Добавьте команду в массив `available_commands`
3. Обновите функцию `kernel_execute_command`

### Добавление нового драйвера
1. Создайте файл драйвера в `drivers/`
2. Реализуйте функции инициализации и работы
3. Добавьте заголовочный файл
4. Интегрируйте в ядро
5. Обновите Makefile

### Добавление новой файловой системы
1. Реализуйте функции чтения/записи
2. Создайте структуру данных для файловой системы
3. Интегрируйте в существующую систему
4. Добавьте поддержку монтирования

---

**Разработка под LakOS** - это отличная возможность изучить низкоуровневое программирование и архитектуру операционных систем!