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
CFLAGS = -m32 -ffreestanding -O0 -Wall -Wextra -I. -Ikernel -Ikernel/include -Ikernel/drivers
LDFLAGS = -m elf_i386 -T linker.ld
OBJ = \
      kernel/start.o \
       kernel/kernel.o \
      kernel/shell.o \
      kernel/users.o \
      kernel/crypt.o \
      kernel/fs/tar.o \
      kernel/gdt.o \
      kernel/idt.o \
      kernel/isr.o \
      kernel/drivers/ata.o \
      kernel/idt_flush.o \
      kernel/interrupts.o \
      kernel/idt_load.o \
      kernel/gdtflush.o

all: lakos.bin modules.tar

lakos.bin: $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

.PHONY: modules.tar
modules.tar:
	cd rootfs && chmod -R +x bin && tar  -cf ../$@ *

iso: lakos.bin modules.tar
	mkdir -p isodir/boot/grub
	cp lakos.bin isodir/boot/
	cp modules.tar isodir/boot/
	echo 'set timeout=0' > isodir/boot/grub/grub.cfg
	echo 'menuentry "Lakos OS" {' >> isodir/boot/grub/grub.cfg
	echo '  multiboot /boot/lakos.bin' >> isodir/boot/grub/grub.cfg
	echo '  module /boot/modules.tar' >> isodir/boot/grub/grub.cfg
	echo '}' >> isodir/boot/grub/grub.cfg
	grub-mkrescue -o lakos.iso isodir

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) -f elf32 $< -o $@

clean:
	rm -f $(OBJ) lakos.bin modules.tar lakos.iso
	rm -rf isodir
