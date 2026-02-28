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
PROJECT_ROOT := $(abspath .)
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
      kernel/drivers/rtl8139.o \
      kernel/drivers/tcpip.o \
      kernel/idt_flush.o \
      kernel/interrupts.o \
      kernel/idt_load.o \
      kernel/gdtflush.o \
      modules.o

USER_CFLAGS = -m32 -ffreestanding -O0 -fno-pie -no-pie
USER_LDFLAGS = -m elf_i386 -e main -Ttext 0x200000 --unresolved-symbols=ignore-all

USER_BIN = rootfs/bin/calc

LIMINE = $(firstword $(wildcard /usr/bin/limine /usr/local/bin/limine $(PROJECT_ROOT)/tools/limine-install/bin/limine) limine)
LIMINE_BIOS_CD = $(firstword $(wildcard \
	/usr/share/limine/limine-bios-cd.bin \
	/usr/local/share/limine/limine-bios-cd.bin \
	/usr/local/share/limine-bootloader/limine-bios-cd.bin \
	$(PROJECT_ROOT)/tools/limine-install/share/limine/limine-bios-cd.bin))
LIMINE_UEFI_CD = $(firstword $(wildcard \
	/usr/share/limine/limine-uefi-cd.bin \
	/usr/local/share/limine/limine-uefi-cd.bin \
	/usr/local/share/limine-bootloader/limine-uefi-cd.bin \
	$(PROJECT_ROOT)/tools/limine-install/share/limine/limine-uefi-cd.bin))
LIMINE_BIOS_SYS = $(firstword $(wildcard \
	/usr/share/limine/limine-bios.sys \
	/usr/local/share/limine/limine-bios.sys \
	/usr/local/share/limine-bootloader/limine-bios.sys \
	$(PROJECT_ROOT)/tools/limine-install/share/limine/limine-bios.sys \
	$(PROJECT_ROOT)/tools/limine-10.7.0/bin/limine-bios.sys))
LIMINE_BOOTX64 = $(firstword $(wildcard \
	/usr/share/limine/BOOTX64.EFI \
	/usr/local/share/limine/BOOTX64.EFI \
	/usr/local/share/limine-bootloader/BOOTX64.EFI \
	$(PROJECT_ROOT)/tools/limine-install/share/limine/BOOTX64.EFI))
LIMINE_BOOTIA32 = $(firstword $(wildcard \
	/usr/share/limine/BOOTIA32.EFI \
	/usr/local/share/limine/BOOTIA32.EFI \
	/usr/local/share/limine-bootloader/BOOTIA32.EFI \
	$(PROJECT_ROOT)/tools/limine-install/share/limine/BOOTIA32.EFI))

# By default build full bootable artifact set (kernel + modules + ISO)
all: iso

lakos.bin: $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

.PHONY: all iso run modules.tar user_programs clean
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
	@test -f "$(LIMINE_BIOS_CD)" || (echo "Missing $(LIMINE_BIOS_CD). Install Limine package." && exit 1)
	@test -f "$(LIMINE_UEFI_CD)" || (echo "Missing $(LIMINE_UEFI_CD). Install Limine package." && exit 1)
	@test -f "$(LIMINE_BIOS_SYS)" || (echo "Missing $(LIMINE_BIOS_SYS). Install Limine package." && exit 1)
	@test -f "$(LIMINE_BOOTX64)" || (echo "Missing $(LIMINE_BOOTX64). Install Limine package." && exit 1)
	mkdir -p isodir/boot/limine isodir/EFI/BOOT
	cp lakos.bin isodir/boot/
	cp modules.tar isodir/boot/
	cp boot/limine.conf isodir/
	cp "$(LIMINE_BIOS_CD)" isodir/boot/limine/
	cp "$(LIMINE_UEFI_CD)" isodir/boot/limine/
	cp "$(LIMINE_BIOS_SYS)" isodir/boot/limine/
	cp "$(LIMINE_BOOTX64)" isodir/EFI/BOOT/BOOTX64.EFI
	@if [ -f "$(LIMINE_BOOTIA32)" ]; then cp "$(LIMINE_BOOTIA32)" isodir/EFI/BOOT/BOOTIA32.EFI; fi
	xorriso -as mkisofs \
		-b boot/limine/limine-bios-cd.bin \
		-no-emul-boot \
		-boot-load-size 4 \
		-boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part \
		--efi-boot-image \
		--protective-msdos-label \
		isodir -o lakos.iso
	@if command -v $(LIMINE) >/dev/null 2>&1; then \
		$(LIMINE) bios-install lakos.iso 2>/dev/null || $(LIMINE) deploy lakos.iso; \
	elif command -v limine-deploy >/dev/null 2>&1; then \
		limine-deploy lakos.iso; \
	else \
		echo "Limine tool not found (expected 'limine' or 'limine-deploy')."; \
		exit 1; \
	fi

run: iso
	qemu-system-i386 -cdrom lakos.iso -boot d -m 512M -nographic

# Run with network support
run-net: iso
	qemu-system-i386 -cdrom lakos.iso -boot d -m 512M  \
		-netdev user,id=net0,hostfwd=tcp::8080-:80 \
		-device rtl8139,netdev=net0

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) -f elf32 $< -o $@

modules.o: modules.tar
	$(OBJCOPY) -I binary -O elf32-i386 -B i386 $< $@

clean:
	rm -f $(OBJ) modules.o lakos.bin modules.tar lakos.iso
	rm -rf isodir