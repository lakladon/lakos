# DRIVERS.md - Драйверы (VGA, клавиатура, диски)

Этот документ описывает реализацию драйверов устройств в LakOS.

## 🖥️ VGA драйвер (kernel/vga.c)

### Общее описание

VGA драйвер в LakOS реализует **текстовый режим 80x25** с поддержкой цветов и графических примитивов.

### Режимы работы

#### Текстовый режим (0x03)
- **Разрешение:** 80x25 символов
- **Цвета:** 16 foreground + 16 background
- **Память:** 0xB8000 (32KB)
- **Формат:** 2 байта на символ (символ + атрибуты)

#### Графический режим (0x13)
- **Разрешение:** 320x200 пикселей
- **Цвета:** 256 цветов (8-bit)
- **Память:** 0xA0000 (64KB)
- **Формат:** 1 байт на пиксель

### Реализация

#### Инициализация VGA
```c
void vga_set_text_mode() {
    // Установка текстового режима 80x25
    __asm__ volatile("mov $0x03, %%ah; int $0x10" : : : "ah");
}
```

#### Текстовый вывод
```c
// Глобальные переменные
uint16_t* video_memory = (uint16_t*)VIDEO_MEMORY;
int term_col = 0;
int term_row = 0;
uint8_t current_attr = 0x0F; // white on black

void terminal_initialize() {
    term_col = 0;
    term_row = 0;
    // Очистка экрана
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        video_memory[i] = (uint16_t)' ' | (uint16_t)0x0F << 8;
    }
}

void terminal_putchar(char c) {
    if (c == '\n') {
        term_col = 0;
        term_row++;
    } else if (c == '\b') {
        if (term_col > 0) {
            term_col--;
            video_memory[term_row * VGA_WIDTH + term_col] = 
                (uint16_t)' ' | (uint16_t)current_attr << 8;
        }
    } else {
        video_memory[term_row * VGA_WIDTH + term_col] = 
            (uint16_t)c | (uint16_t)current_attr << 8;
        term_col++;
    }

    // Перенос строки
    if (term_col >= VGA_WIDTH) {
        term_col = 0;
        term_row++;
    }

    // Прокрутка экрана
    if (term_row >= VGA_HEIGHT) {
        for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
            video_memory[i] = video_memory[i + VGA_WIDTH];
        }
        // Очистка последней строки
        for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
            video_memory[i] = (uint16_t)' ' | (uint16_t)current_attr << 8;
        }
        term_row = VGA_HEIGHT - 1;
    }
}
```

#### Цветной вывод
```c
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

#### Графические примитивы
```c
// Рисование пикселя
void vga_put_pixel(int x, int y, uint8_t color) {
    uint8_t* vga = (uint8_t*)VGA_MEMORY;
    vga[y * 320 + x] = color;
}

// Рисование линии (алгоритм Брезенхема)
void vga_draw_line(int x1, int y1, int x2, int y2, uint8_t color) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = (abs(dx) > abs(dy)) ? abs(dx) : abs(dy);
    float xinc = dx / (float)steps;
    float yinc = dy / (float)steps;
    float x = x1;
    float y = y1;
    for (int i = 0; i <= steps; i++) {
        vga_put_pixel((int)x, (int)y, color);
        x += xinc;
        y += yinc;
    }
}

// Рисование прямоугольника
void vga_draw_rectangle(int x, int y, int w, int h, uint8_t color) {
    for (int i = x; i < x + w; i++) {
        vga_put_pixel(i, y, color);
        vga_put_pixel(i, y + h - 1, color);
    }
    for (int i = y; i < y + h; i++) {
        vga_put_pixel(x, i, color);
        vga_put_pixel(x + w - 1, i, color);
    }
}

// Заливка прямоугольника
void vga_fill_rectangle(int x, int y, int w, int h, uint8_t color) {
    for (int i = y; i < y + h; i++) {
        for (int j = x; j < x + w; j++) {
            vga_put_pixel(j, i, color);
        }
    }
}
```

#### Шрифт
```c
// 8x8 шрифт для ASCII 32-126
const uint8_t font[95][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // space
    {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00}, // !
    // ... остальные символы
};

void vga_draw_char(int x, int y, char c, uint8_t color) {
    if (c < 32 || c > 126) return;
    uint8_t* bitmap = (uint8_t*)font[c - 32];
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (bitmap[i] & (1 << (7 - j))) {
                vga_put_pixel(x + j, y + i, color);
            }
        }
    }
}

void vga_draw_text(int x, int y, const char* text, uint8_t color) {
    int cx = x;
    while (*text) {
        if (*text == '\n') {
            y += 8;
            cx = x;
        } else {
            vga_draw_char(cx, y, *text, color);
            cx += 8;
        }
        text++;
    }
}
```

## ⌨️ Клавиатурный драйвер

### Общее описание

Клавиатурный драйвер обрабатывает **PS/2** скан-коды и преобразует их в ASCII символы.

### Порты ввода/вывода
- **0x60** - Data port (данные)
- **0x64** - Status port (статус)

### Скан-коды
```c
unsigned char kbd_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0
};

unsigned char kbd_map_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0
};
```

### Обработка прерываний
```c
static int shift_pressed = 0;
static int caps_locked = 0;

void read_line(char* buffer, int max, int echo) {
    int ptr = 0;
    while (1) {
        if (inb(0x64) & 0x1) {
            uint8_t scancode = inb(0x60);
            
            // Обработка модификаторов
            if ((scancode & 0x7F) == 42 || (scancode & 0x7F) == 54) {
                shift_pressed = !(scancode & 0x80);
            } else if (scancode == 58) {
                if (!(scancode & 0x80)) caps_locked = !caps_locked;
            } else if (!(scancode & 0x80)) {
                // Преобразование скан-кода в символ
                int is_letter = (scancode >= 16 && scancode <= 25) || 
                               (scancode >= 30 && scancode <= 38) || 
                               (scancode >= 44 && scancode <= 50);
                int uppercase = shift_pressed || (caps_locked && is_letter);
                char c = uppercase ? kbd_map_shift[scancode] : kbd_map[scancode];
                
                if (c == '\n') {
                    buffer[ptr] = '\0';
                    if (echo) terminal_putchar('\n');
                    return;
                } else if (c == '\b') {
                    if (ptr > 0) {
                        ptr--;
                        if (echo) {
                            terminal_putchar('\b');
                            terminal_putchar(' ');
                            terminal_putchar('\b');
                        }
                    }
                } else if (ptr < max && c != 0) {
                    buffer[ptr++] = c;
                    if (echo) terminal_putchar(c);
                }
            }
        }
    }
}
```

## 💾 ATA драйвер (kernel/drivers/ata.c)

### Общее описание

ATA драйвер реализует **IDE** интерфейс для работы с жесткими дисками.

### Порты ввода/вывода
```c
#define ATA_DATA 0x1F0
#define ATA_FEATURES 0x1F1
#define ATA_SECTOR_COUNT 0x1F2
#define ATA_LBA_LOW 0x1F3
#define ATA_LBA_MID 0x1F4
#define ATA_LBA_HIGH 0x1F5
#define ATA_DRIVE 0x1F6
#define ATA_COMMAND 0x1F7
#define ATA_STATUS 0x1F7
```

### Команды
```c
#define ATA_CMD_READ 0x20
#define ATA_CMD_WRITE 0x30
#define ATA_CMD_IDENTIFY 0xEC
```

### Реализация

#### Ожидание готовности
```c
int ata_wait() {
    int timeout = 100000;
    while ((inb(ATA_STATUS) & 0x80) && timeout--); // Wait for BSY to clear
    return timeout > 0;
}
```

#### Выбор диска
```c
void ata_select_drive(uint8_t drive) {
    outb(ATA_DRIVE, 0xE0 | (drive << 4));
}
```

#### Идентификация диска
```c
int ata_identify(uint8_t drive) {
    ata_select_drive(drive);
    outb(ATA_SECTOR_COUNT, 0);
    outb(ATA_LBA_LOW, 0);
    outb(ATA_LBA_MID, 0);
    outb(ATA_LBA_HIGH, 0);
    outb(ATA_COMMAND, ATA_CMD_IDENTIFY);

    uint8_t status = inb(ATA_STATUS);
    if (status == 0) {
        return 0; // No drive
    }

    if (!ata_wait()) {
        return 0; // Timeout
    }
    status = inb(ATA_STATUS);
    if (status & 0x01) {
        return 0; // Error
    }

    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(ATA_DATA);
    }

    // Debug output
    terminal_writestring("ATA Drive ");
    char buf[2];
    buf[0] = '0' + drive;
    buf[1] = 0;
    terminal_writestring(buf);
    terminal_writestring(" ID: 0x");
    print_hex(identify_data[0], 4);
    terminal_writestring("\n");

    return 1;
}
```

#### Чтение сектора
```c
void ata_read_sector(uint8_t drive, uint32_t lba, uint16_t* buffer) {
    if (drive > 1) return;
    if (lba > 0xFFFFFF) return; // LBA28 limit
    
    // Debug output
    terminal_writestring("DEBUG: Reading sector ");
    char buf[16];
    itoa(lba, buf);
    terminal_writestring(buf);
    terminal_writestring(" from drive ");
    itoa(drive, buf);
    terminal_writestring(buf);
    terminal_writestring("\n");
    
    ata_select_drive(drive);
    outb(ATA_SECTOR_COUNT, 1);
    outb(ATA_LBA_LOW, lba & 0xFF);
    outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(ATA_COMMAND, ATA_CMD_READ);

    if (!ata_wait()) {
        terminal_writestring("DEBUG: ATA read timeout\n");
        return;
    }
    uint8_t status = inb(ATA_STATUS);
    if (status & 0x01) {
        terminal_writestring("DEBUG: ATA read error - ERR bit set\n");
        return;
    }
    
    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(ATA_DATA);
    }
    
    terminal_writestring("DEBUG: ATA read completed successfully\n");
}
```

#### Запись сектора
```c
void ata_write_sector(uint8_t drive, uint32_t lba, uint16_t* buffer) {
    if (drive > 1) return;
    if (lba > 0xFFFFFF) return; // LBA28 limit
    ata_select_drive(drive);
    outb(ATA_SECTOR_COUNT, 1);
    outb(ATA_LBA_LOW, lba & 0xFF);
    outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(ATA_COMMAND, ATA_CMD_WRITE);

    if (!ata_wait()) return;
    for (int i = 0; i < 256; i++) {
        outw(ATA_DATA, buffer[i]);
    }
    // Flush cache
    ata_wait();
}
```

#### Чтение нескольких секторов
```c
void ata_read_sectors(uint8_t drive, uint32_t lba, uint16_t* buffer, uint8_t count) {
    ata_select_drive(drive);
    outb(ATA_SECTOR_COUNT, count);
    outb(ATA_LBA_LOW, lba & 0xFF);
    outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(ATA_COMMAND, ATA_CMD_READ);

    if (!ata_wait()) return;
    for (int s = 0; s < count; s++) {
        for (int i = 0; i < 256; i++) {
            buffer[s * 256 + i] = inw(ATA_DATA);
        }
    }
}
```

#### Инициализация
```c
void ata_init() {
    // Initialize ATA
}

int ata_detect_disks() {
    int count = 0;
    if (ata_identify(0)) count++;
    if (ata_identify(1)) count++;
    return count;
}
```

## 🖱️ Мышь (kernel/drivers/mouse.c)

### Общее описание

Драйвер мыши реализует **PS/2** протокол для двухкнопочной мыши.

### Порты ввода/вывода
- **0x60** - Data port
- **0x64** - Command port

### Реализация

#### Инициализация мыши
```c
void mouse_install() {
    // Enable mouse
    outb(0x64, 0xA8);
    
    // Enable mouse interrupts
    outb(0x64, 0x20);
    uint8_t status = inb(0x60);
    status |= 0x02;
    outb(0x64, 0x60);
    outb(0x60, status);
    
    // Mouse command: enable
    outb(0x64, 0xD4);
    outb(0x60, 0xF4);
}
```

#### Обработка прерываний
```c
// Mouse buffer
static uint8_t mouse_buffer[3];
static int mouse_byte = 0;

void mouse_handler() {
    uint8_t data = inb(0x60);
    
    mouse_buffer[mouse_byte] = data;
    mouse_byte++;
    
    if (mouse_byte == 3) {
        // Process mouse packet
        uint8_t status = mouse_buffer[0];
        int8_t x = mouse_buffer[1];
        int8_t y = mouse_buffer[2];
        
        // Handle mouse movement
        handle_mouse_movement(status, x, y);
        
        mouse_byte = 0;
    }
}
```

## 🔧 Ввод/вывод (kernel/drivers/io.h)

### Базовые функции
```c
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}
```

## 📊 Производительность драйверов

### VGA драйвер
- **Текстовый режим:** Высокая производительность
- **Графический режим:** Средняя производительность
- **Цвета:** 16 foreground + 16 background

### ATA драйвер
- **Скорость чтения:** ~1-2 MB/s (ограничено эмулятором)
- **LBA поддержка:** До 28 бит (137GB)
- **Ошибки:** Базовая обработка

### Клавиатурный драйвер
- **Реакция:** Мгновенная
- **Поддержка:** PS/2, модификаторы
- **Буфер:** 256 символов

### Мышь
- **Протокол:** PS/2
- **Кнопки:** 2-3 кнопки
- **Чувствительность:** Стандартная

## 🚀 Возможные улучшения

### VGA драйвер
- Поддержка VESA BIOS Extensions
- Улучшенные шрифты
- Аппаратное ускорение
- Поддержка мыши в графическом режиме

### ATA драйвер
- Поддержка LBA48
- DMA режим
- SATA поддержка
- SMART атрибуты

### Клавиатурный драйвер
- Поддержка USB HID
- Расширенные модификаторы
- Макросы
- Раскладки

### Мышь
- Поддержка колеса
- USB HID
- Разрешение (DPI)
- Дополнительные кнопки

---

**Драйверы LakOS** - это базовая, но функциональная реализация для работы с основными устройствами ввода-вывода.