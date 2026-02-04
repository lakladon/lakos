# BOOT_SEQUENCE.md - Процесс загрузки LakOS

Этот документ подробно описывает процесс загрузки операционной системы LakOS от момента включения компьютера до запуска shell.

## 🔄 Общая последовательность загрузки

```
1. BIOS → GRUB
   ↓
2. GRUB → boot.asm (Multiboot)
   ↓
3. boot.asm → kernel_entry
   ↓
4. kernel_main()
   ├── Инициализация GDT
   ├── Инициализация IDT
   ├── Инициализация VGA
   ├── Загрузка tar-fs
   └── Запуск shell
   ↓
5. Shell → Пользовательский ввод
```

## 📋 Детальный процесс загрузки

### Этап 1: BIOS и GRUB

#### BIOS (Basic Input/Output System)
**Функции:**
- Проверка аппаратуры (POST)
- Инициализация базовых устройств
- Поиск загрузочного устройства
- Загрузка MBR (Master Boot Record)

#### GRUB (Grand Unified Bootloader)
**Функции:**
- Загрузка Multiboot-совместимых ОС
- Передача управления ядру
- Предоставление информации о системе

**Multiboot заголовок:**
```c
// kernel/kernel.c
const multiboot_header_t __attribute__((section(".multiboot"))) header = {
    .magic = MULTIBOOT_HEADER_MAGIC,
    .flags = 0,
    .checksum = -(MULTIBOOT_HEADER_MAGIC + 0)
};
```

### Этап 2: Загрузчик (boot/boot.asm)

#### Переход в защищенный режим
```asm
; Включение защищенного режима
cli                     ; Отключаем прерывания
lgdt [gdt_descriptor]   ; Загружаем GDT
mov eax, cr0
or eax, 1               ; Устанавливаем бит PE
mov cr0, eax

; Переход в защищенный режим
jmp 0x08:start          ; Переход на сегмент кода
```

#### Инициализация сегментов
```asm
start:
    mov ax, 0x10        ; Сегмент данных
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000    ; Стек в 576KB

    ; Передача управления ядру
    call kernel_main
```

### Этап 3: Главная функция ядра (kernel/kernel.c)

#### Инициализация системных таблиц

##### 3.1 Global Descriptor Table (GDT)
```c
void init_gdt(void) {
    gdt_ptr.limit = sizeof(struct gdt_entry) * 3 - 1;
    gdt_ptr.base = (uint32_t)&gdt_entries;
    
    // Нулевой дескриптор
    gdt_set_gate(0, 0, 0, 0, 0);
    
    // Кодовый дескриптор (0x08)
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    
    // Данный дескриптор (0x10)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    
    // Загрузка GDT
    gdt_flush();
}
```

##### 3.2 Interrupt Descriptor Table (IDT)
```c
void idt_init(void) {
    idt_ptr.limit = sizeof(struct idt_entry) * 256 - 1;
    idt_ptr.base = (uint32_t)&idt_entries;
    
    // Установка обработчиков прерываний
    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, (uint32_t)isr_stub_table[i], 0x08, 0x8E);
    }
    
    // Загрузка IDT
    idt_flush();
}
```

##### 3.3 Установка обработчиков прерываний
```c
void irq_install(void) {
    // Инициализация PIC
    outb(0x20, 0x11);     // ICW1: Edge triggered mode
    outb(0x21, 0x20);     // ICW2: Master PIC vector offset
    outb(0x21, 0x04);     // ICW3: Slave PIC at IRQ2
    outb(0x21, 0x01);     // ICW4: 8086 mode
    
    // Установка обработчиков IRQ
    for (int i = 32; i < 48; i++) {
        idt_set_gate(i, (uint32_t)irq_stub_table[i-32], 0x08, 0x8E);
    }
}
```

#### 3.4 Инициализация VGA
```c
void vga_set_text_mode(void) {
    // Установка текстового режима 80x25
    __asm__ volatile("mov $0x03, %%ah; int $0x10" : : : "ah");
}
```

#### 3.5 Загрузка файловой системы
```c
// Получение указателя на tar-архив
extern char _binary_modules_tar_start[];
void* tar_archive = (void*)&_binary_modules_tar_start;
```

#### 3.6 Инициализация драйверов
```c
// Инициализация ATA контроллера
ata_init();

// Обнаружение дисков
int disk_count = ata_detect_disks();

// Инициализация мыши
mouse_install();
```

#### 3.7 Инициализация пользовательской системы
```c
// Инициализация пользователей
init_users();

// Инициализация команд ядра
init_kernel_commands();
```

### Этап 4: Запуск shell

#### 4.1 Включение прерываний
```c
__asm__ volatile("sti");
```

#### 4.2 Запуск shell
```c
shell_main();
```

#### 4.3 Цикл обработки команд
```c
void shell_main(void) {
    // Инициализация shell
    terminal_initialize();
    
    // Авторизация пользователя
    authenticate_user();
    
    // Основной цикл
    while (1) {
        // Обработка ввода
        // Выполнение команд
        // Обработка прерываний
    }
}
```

## 🎨 Boot-анимация

### Красивая загрузка с системной информацией

#### Логотип системы
```c
void boot_draw_logo(void) {
    terminal_writestring("\033[36m"); // Cyan color
    terminal_writestring("  _______      __  __  __          __  __\n");
    terminal_writestring(" /_  __(_)____/ /_/ /_/ /__  _____/ /_/ /__  ____  ____\n");
    terminal_writestring("  / / / / ___/ __/ __/ / _ \\/ ___/ __/ / _ \\/ __ \\/ __ \\\n");
    terminal_writestring(" / / / (__  ) /_/ /_/ /  __/ /  / /_/ /  __/ /_/ / /_/ /\n");
    terminal_writestring("/_/ /_/____/\\__/\\__/_/\\___/_/   \\__/_/\\___/ .___/\\____/\n");
    terminal_writestring("                                        /_/           \n");
    terminal_writestring("\033[0m"); // Reset color
}
```

#### Прогресс-бар загрузки
```c
void boot_draw_progress_bar(int progress, int max) {
    terminal_writestring("\r["); // Carriage return to overwrite
    for (int i = 0; i < 20; i++) {
        if (i < (progress * 20) / max) {
            terminal_writestring("=");
        } else {
            terminal_writestring(" ");
        }
    }
    terminal_writestring("] ");
    char buf[10];
    itoa((progress * 100) / max, buf);
    terminal_writestring(buf);
    terminal_writestring("%");
}
```

#### Постепенное появление текста
```c
void boot_fade_in_text(const char* text, int delay) {
    for (int i = 0; i < 3; i++) {
        terminal_writestring("\n");
    }
    for (int i = 0; i < 3; i++) {
        terminal_writestring(text);
        terminal_writestring("\n");
        for (int j = 0; j < delay; j++) {
            for (volatile int k = 0; k < 10000; k++);
        }
    }
}
```

### Системная информация

#### Сбор информации о системе
```c
typedef struct {
    uint32_t memory_size;
    uint32_t cpu_features;
    char kernel_version[16];
    char build_date[32];
} system_info_t;

system_info_t sys_info;

// Определение объема памяти
uint32_t detect_memory_size(multiboot_info_t* mb_info) {
    if (mb_info->flags & 0x0001) {
        return mb_info->mem_upper * 1024;
    }
    return 0;
}

// Определение возможностей CPU
uint32_t detect_cpu_features(void) {
    uint32_t features = 0;
    uint32_t eax, ebx, ecx, edx;
    
    // Проверка поддержки CPUID
    __asm__ volatile(
        "pushf\n\t"
        "pop %%eax\n\t"
        "mov %%eax, %%ebx\n\t"
        "xor $0x200000, %%eax\n\t"
        "push %%eax\n\t"
        "popf\n\t"
        "pushf\n\t"
        "pop %%eax\n\t"
        "xor %%ebx, %%eax\n\t"
        "mov %%eax, %0\n\t"
        : "=r"(features)
        :
        : "eax", "ebx"
    );
    
    return features;
}
```

#### Вывод системной информации
```c
void display_system_info(void) {
    terminal_writestring("\n\033[33mSystem Information:\033[0m\n");
    terminal_writestring("Kernel Version: ");
    terminal_writestring(sys_info.kernel_version);
    terminal_writestring("\n");
    terminal_writestring("Build Date: ");
    terminal_writestring(sys_info.build_date);
    terminal_writestring("\n");
    terminal_writestring("Memory: ");
    char buf[10];
    itoa(sys_info.memory_size / 1024, buf);
    terminal_writestring(buf);
    terminal_writestring(" KB\n");
    terminal_writestring("CPU Features: ");
    itoa(sys_info.cpu_features, buf);
    terminal_writestring(buf);
    terminal_writestring("\n\n");
}
```

## ⚡ Оптимизации загрузки

### 1. Минимизация времени загрузки
- Простые алгоритмы инициализации
- Отсутствие избыточных проверок
- Прямое использование аппаратуры

### 2. Эффективное использование памяти
- Минимальный размер ядра
- Оптимизация структур данных
- Отсутствие ненужных абстракций

### 3. Надежность загрузки
- Проверка целостности компонентов
- Обработка ошибок инициализации
- Отказоустойчивость

## 🔧 Отладка загрузки

### 1. Вывод отладочной информации
```c
#ifdef DEBUG
#define BOOT_DEBUG(msg) \
    do { \
        terminal_writestring("[BOOT] "); \
        terminal_writestring(msg); \
        terminal_writestring("\n"); \
    } while (0)
#else
#define BOOT_DEBUG(msg)
#endif
```

### 2. Пошаговая инициализация
```c
void debug_boot_sequence(void) {
    BOOT_DEBUG("Starting boot sequence");
    
    BOOT_DEBUG("Initializing GDT");
    init_gdt();
    
    BOOT_DEBUG("Initializing IDT");
    idt_init();
    
    BOOT_DEBUG("Installing IRQ handlers");
    irq_install();
    
    BOOT_DEBUG("Loading filesystem");
    tar_archive = (void*)&_binary_modules_tar_start;
    
    BOOT_DEBUG("Boot sequence completed");
}
```

### 3. Обработка ошибок
```c
void handle_boot_error(const char* error_msg) {
    terminal_writestring("\033[31mBoot Error: ");
    terminal_writestring(error_msg);
    terminal_writestring("\033[0m\n");
    
    // Остановка системы
    while (1) {
        __asm__ volatile("hlt");
    }
}
```

## 📊 Временные характеристики

### Типичное время загрузки
- **BIOS/GRUB:** ~1-2 секунды
- **boot.asm:** ~0.1 секунды
- **kernel_main():** ~1-2 секунды
- **Общее время:** ~3 секунды

### Факторы, влияющие на время загрузки
- Скорость процессора
- Объем памяти
- Скорость диска
- Количество инициализируемых устройств

## 🚀 Возможные улучшения

### 1. Ускорение загрузки
- Оптимизация алгоритмов инициализации
- Параллельная инициализация независимых компонентов
- Минимизация размера ядра

### 2. Улучшение boot-анимации
- Более сложные визуальные эффекты
- Анимация прогресс-бара
- Динамическое отображение системной информации

### 3. Расширенная диагностика
- Подробные логи загрузки
- Автоматическое определение проблем
- Рекомендации по устранению неполадок

---

**Процесс загрузки LakOS** - это тщательно продуманная последовательность шагов, обеспечивающая надежный и быстрый запуск системы.