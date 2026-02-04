# BUILDING.md - Детальное руководство по сборке

Этот документ содержит подробную информацию о процессе сборки LakOS, включая все этапы, зависимости и настройки.

## 📋 Системные требования

### Минимальные требования

- **ОС:** Linux, macOS, Windows (через WSL2)
- **Процессор:** i386 совместимый
- **Память:** 1GB RAM
- **Место на диске:** 100MB свободного места

### Необходимые инструменты

#### Компилятор и ассемблер
```bash
# Для Linux (Ubuntu/Debian)
sudo apt install gcc gcc-multilib nasm

# Для macOS
brew install gcc nasm

# Для Windows (WSL2)
sudo apt install gcc gcc-multilib nasm
```

#### Инструменты для создания ISO
```bash
# Ubuntu/Debian
sudo apt install xorriso grub-pc-bin

# macOS
brew install xorriso
# GRUB можно установить через brew или использовать другие средства
```

#### Эмуляторы (для тестирования)
```bash
# QEMU (рекомендуется)
sudo apt install qemu-system-i386  # Linux
brew install qemu                  # macOS

# VirtualBox (альтернатива)
# Скачать с официального сайта
```

## 🏗️ Структура Makefile

Проект использует GNU Make для автоматизации сборки. Основные цели:

```makefile
# Основные цели
all: kernel rootfs iso        # Собрать всё
clean:                       # Очистить артефакты
distclean: clean             # Полная очистка

# Поэтапная сборка
kernel: lakos.bin           # Собрать ядро
rootfs: modules.tar          # Собрать rootfs
iso: lakos.iso              # Создать ISO

# Вспомогательные цели
test: iso                   # Собрать и запустить в QEMU
debug:                      # Собрать с отладочной информацией
```

## 🔧 Процесс сборки

### Этап 1: Сборка ядра

**Цель:** `make kernel` или `make lakos.bin`

**Процесс:**
1. Компиляция C-файлов в объектные файлы
2. Ассемблирование ASM-файлов
3. Компоновка в единый бинарный образ

**Команды:**
```bash
# Компиляция C-файлов
gcc -m32 -ffreestanding -fno-exceptions -fno-stack-protector \
    -fno-builtin -nostdlib -nostdinc -nostartfiles \
    -I kernel/include/ -c kernel/kernel.c -o kernel/kernel.o

# Ассемблирование
nasm -f elf32 boot/boot.asm -o boot/boot.o

# Компоновка
ld -m elf_i386 -T linker.ld -o lakos.bin \
    boot/boot.o kernel/kernel.o kernel/drivers/*.o \
    kernel/fs/*.o kernel/*.o
```

**Особенности:**
- Используется кросс-компиляция для i386
- Отключены стандартные библиотеки
- Используется custom linker script
- Результат: `lakos.bin` (~100KB)

### Этап 2: Сборка RootFS

**Цель:** `make rootfs` или `make modules.tar`

**Процесс:**
1. Создание временной файловой системы
2. Копирование файлов из `rootfs/`
3. Архивирование в tar-формат
4. Сжатие (опционально)

**Команды:**
```bash
# Создание архива
tar -cf modules.tar -C rootfs/ .

# Сжатие (если нужно)
gzip -c modules.tar > modules.tar.gz
```

**Структура rootfs:**
```
rootfs/
├── bin/           # Пользовательские программы
│   ├── calc       # Калькулятор
│   └── calc.c     # Исходный код
├── etc/           # Конфигурация
├── home/          # Домашние директории
└── test.txt       # Тестовые файлы
```

### Этап 3: Создание ISO

**Цель:** `make iso` или `make lakos.iso`

**Процесс:**
1. Создание временной директории ISO
2. Копирование ядра и rootfs
3. Создание загрузочной структуры
4. Генерация ISO образа

**Команды:**
```bash
# Создание временной директории
mkdir -p iso/boot/grub

# Копирование файлов
cp lakos.bin iso/boot/
cp modules.tar iso/boot/
cp grub.cfg iso/boot/grub/

# Создание ISO
grub-mkrescue -o lakos.iso iso/ --xorriso xorriso
```

**Структура ISO:**
```
iso/
├── boot/
│   ├── lakos.bin      # Ядро
│   ├── modules.tar    # RootFS
│   └── grub/
│       └── grub.cfg   # Конфигурация GRUB
└── README             # Документация
```

## ⚙️ Конфигурация сборки

### Переменные Makefile

```makefile
# Архитектура
ARCH = i386

# Компилятор
CC = i686-elf-gcc
AS = nasm
LD = i686-elf-ld

# Флаги компиляции
CFLAGS = -m32 -ffreestanding -O0 -Wall -Wextra \
         -I. -Ikernel -Ikernel/include -Ikernel/drivers

# Флаги линковки
LDFLAGS = -m elf_i386 -T linker.ld

# Цели
TARGETS = lakos.bin modules.tar lakos.iso
```

### Кастомизация

#### Изменение размера памяти
```makefile
# В linker.ld изменить размер памяти
MEMORY {
    RAM (rwx) : ORIGIN = 0x100000, LENGTH = 16M
}
```

#### Добавление новых драйверов
```makefile
# В Makefile добавить новые объектные файлы
KERNEL_OBJS = kernel/kernel.o kernel/drivers/vga.o \
              kernel/drivers/ata.o kernel/drivers/mouse.o \
              kernel/drivers/new_driver.o
```

#### Настройка rootfs
```bash
# Добавить новые файлы в rootfs
cp my_program rootfs/bin/
cp my_config rootfs/etc/

# Пересобрать rootfs
make rootfs
```

## 🐛 Отладка сборки

### Распространенные проблемы

#### Проблема 1: "gcc: command not found"
**Решение:**
```bash
# Ubuntu/Debian
sudo apt install gcc gcc-multilib

# macOS
brew install gcc
```

#### Проблема 2: "nasm: command not found"
**Решение:**
```bash
# Ubuntu/Debian
sudo apt install nasm

# macOS
brew install nasm
```

#### Проблема 3: "grub-mkrescue: command not found"
**Решение:**
```bash
# Ubuntu/Debian
sudo apt install grub-pc-bin

# macOS
# Установить GRUB через brew или использовать альтернативы
```

#### Проблема 4: "ld: cannot find -lgcc"
**Решение:**
```bash
# Установить multilib пакеты
sudo apt install gcc-multilib
```

#### Проблема 5: "No bootable device"
**Решение:**
1. Проверить, что `lakos.iso` был успешно создан
2. Убедиться, что размер ISO > 1MB
3. Проверить структуру ISO-образа

### Отладочные флаги

#### Сборка с отладочной информацией
```makefile
CFLAGS += -g -DDEBUG
```

#### Включение ассемблерного вывода
```makefile
CFLAGS += -S
```

#### Проверка линковки
```bash
# Просмотреть символы в бинарнике
nm lakos.bin

# Проверить секции
objdump -h lakos.bin
```

## 📊 Артефакты сборки

### Ядро (lakos.bin)
- **Размер:** ~100KB
- **Формат:** Raw binary
- **Адрес загрузки:** 0x100000
- **Содержит:** Ядро, драйверы, shell, GUI

### RootFS (modules.tar)
- **Размер:** Зависит от содержимого
- **Формат:** Tar-архив
- **Содержит:** Файловую систему
- **Тип:** Только для чтения

### ISO (lakos.iso)
- **Размер:** ~2MB
- **Формат:** ISO 9660
- **Загрузчик:** GRUB
- **Стандарт:** Multiboot

## 🔍 Анализ бинарных файлов

### Проверка ядра
```bash
# Проверить заголовок Multiboot
hexdump -C lakos.bin | head -20

# Проверить секции
objdump -h lakos.bin

# Проверить символы
nm lakos.bin | head -20
```

### Проверка rootfs
```bash
# Просмотреть содержимое tar
tar -tf modules.tar

# Проверить структуру
tar -tvf modules.tar
```

### Проверка ISO
```bash
# Проверить загрузочность
isoinfo -d -i lakos.iso

# Просмотреть файлы
isoinfo -l -i lakos.iso
```

## 🚀 Автоматизация сборки

### CI/CD Pipeline

Пример GitHub Actions workflow:

```yaml
name: Build LakOS
on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: sudo apt install gcc gcc-multilib nasm xorriso grub-pc-bin
      - name: Build
        run: make all
      - name: Upload artifacts
        uses: actions/upload-artifact@v2
        with:
          name: lakos-artifacts
          path: lakos.iso
```

### Docker сборка

```dockerfile
FROM ubuntu:20.04

RUN apt update && apt install -y \
    gcc gcc-multilib nasm xorriso grub-pc-bin

COPY . /lakos
WORKDIR /lakos
RUN make all

CMD ["qemu-system-i386", "-cdrom", "lakos.iso"]
```

## 📚 Дополнительные ресурсы

- [GNU Make Manual](https://www.gnu.org/software/make/manual/)
- [NASM Documentation](https://www.nasm.us/doc/)
- [GRUB Manual](https://www.gnu.org/software/grub/manual/)
- [QEMU Documentation](https://www.qemu.org/documentation/)

---

**Сборка LakOS** - это процесс, требующий внимательности к деталям и понимания низкоуровневых аспектов разработки операционных систем.