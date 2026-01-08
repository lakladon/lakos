# Настройки компилятора
CC = i686-elf-gcc
AS = nasm
LD = i686-elf-ld

# Флаги
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -Ikernel/include
ASFLAGS = -f elf32
LDFLAGS = -T linker.ld -m elf_i386

# Поиск всех исходников
C_SOURCES = $(shell find kernel -name '*.c')
ASM_SOURCES = $(shell find kernel -name '*.asm') boot/boot.asm

# Превращение имен .c и .asm в .o
OBJ = $(C_SOURCES:.c=.o) $(ASM_SOURCES:.asm=.o)

# Имя итогового файла
KERNEL_BIN = lakos.bin
FS_TAR = modules.tar

all: $(KERNEL_BIN) $(FS_TAR)

# Сборка ядра
$(KERNEL_BIN): $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

# Компиляция Си-файлов
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Компиляция Ассемблер-файлов
%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

# Создание образа Файловой Системы
$(FS_TAR):
	@mkdir -p rootfs/bin
	# Если есть готовые бинарники, они попадут сюда
	tar -cvf $(FS_TAR) -C rootfs .

# Очистка проекта
clean:
	rm -rf $(OBJ) $(KERNEL_BIN) $(FS_TAR)

# Запуск в QEMU
run: all
	qemu-system-i386 -kernel $(KERNEL_BIN) -initrd $(FS_TAR)