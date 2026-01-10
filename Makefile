CC = i686-elf-gcc
# Если i686-elf-gcc нет, используем обычный gcc с флагами
ifeq (, $(shell which $(CC)))
    CC = gcc
endif

LD = i686-elf-ld
ifeq (, $(shell which $(LD)))
    LD = ld
endif

AS = nasm
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -I. -Ikernel -Ikernel/include -Ikernel/drivers
LDFLAGS = -m elf_i386 -T linker.ld

OBJ = boot/boot.o \
      kernel/kernel.o \
      kernel/shell.o \
      kernel/fs/tar.o \
      kernel/gdt.o \
      kernel/idt.o \
      kernel/isr.o \
      kernel/drivers/keyboard.o \
      kernel/idt_flush.o \
      kernel/interrupts.o \
      kernel/idt_load.o \
      kernel/gdtflush.o

all: lakos.bin

lakos.bin: $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) -f elf32 $< -o $@

clean:
	rm -f $(OBJ) lakos.bin