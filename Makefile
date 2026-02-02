CC = i686-elf-gcc
# Если i686-elf-gcc нет, используем обычный gcc с флагами
ifeq (, $(shell which $(CC)))
    CC = gcc
endif

LD = i686-elf-ld
ifeq (, $(shell which $(LD)))
    LD = ld
endif

OBJCOPY = i686-elf-objcopy
ifeq (, $(shell which $(OBJCOPY)))
    OBJCOPY = objcopy
endif

AS = nasm
CFLAGS = -m32 -ffreestanding -O0 -Wall -Wextra -I. -Ikernel -Ikernel/include -Ikernel/drivers
LDFLAGS = -m elf_i386 -T linker.ld
OBJ = \
      kernel/start.o \
      kernel/kernel.o \
      kernel/shell.o \
      kernel/lib.o \
      kernel/commands.o \
      kernel/vga.o \
      kernel/gui.o \
      kernel/users.o \
      kernel/crypt.o \
      kernel/fs/tar.o \
      kernel/gdt.o \
      kernel/idt.o \
      kernel/isr.o \
      kernel/drivers/ata.o \
      kernel/drivers/mouse.o \
      kernel/idt_flush.o \
      kernel/interrupts.o \
      kernel/idt_load.o \
      kernel/gdtflush.o \
      modules.o

USER_CFLAGS = -m32 -ffreestanding -O0 -fno-pie -no-pie
USER_LDFLAGS = -m elf_i386 -e main -Ttext 0x200000 --unresolved-symbols=ignore-all

USER_BIN = rootfs/bin/calc

all: lakos.bin

lakos.bin: $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

.PHONY: modules.tar user_programs
modules.tar: user_programs
	rm -f modules.tar
	cd rootfs && find . -type d ! -path . | tar --no-recursion -cf ../$@ --transform 's|^\./||' -T -
	cd rootfs && find . -type f \( -name "*.c" -o -name "*.h" \) | tar -rf ../$@ --transform 's|^\./||' -T -
	cd rootfs && find . -type f ! \( -name "*.c" -o -name "*.h" \) | tar -rf ../$@ --transform 's|^\./||' -T -

user_programs: $(USER_BIN)

rootfs/bin/calc: rootfs/bin/calc.c
	$(CC) $(USER_CFLAGS) -c $< -o /tmp/calc.o
	$(LD) $(USER_LDFLAGS) -o $@ /tmp/calc.o

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

modules.o: modules.tar
	$(OBJCOPY) -I binary -O elf32-i386 -B i386 $< $@

clean:
	rm -f $(OBJ) modules.o lakos.bin modules.tar lakos.iso
	rm -rf isodir
