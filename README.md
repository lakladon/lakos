# Lakos OS

Lakos OS is a simple, experimental operating system written in C and Assembly, designed as a learning project for low-level system development. It features a basic graphical terminal, interrupt handling, mouse support, and a simple shell with filesystem simulation.

## Features

- **Multiboot Compliant**: Boots via GRUB or compatible bootloaders
- **Graphical Terminal**: VGA Mode 13h (320x200) with basic text rendering
- **Interrupt Handling**: GDT, IDT, and ISR setup for hardware interrupts
- **Input Devices**: Keyboard and mouse support
- **Shell**: Command-line interface with built-in commands and program execution
- **Filesystem Simulation**: Basic directory structure with `/bin`, `/dev`, `/home`
- **Programs**: Includes simple programs like `hello`, `test`, and `editor`

## Version

Current version: 0.3.7 (Build 44)

## Building

### Prerequisites

- GCC (with i686-elf cross-compiler support)
- NASM assembler
- GNU Make
- GNU Binutils

For Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install -y gcc-multilib nasm make binutils binutils-i686-linux-gnu
```

If i686-elf-gcc is not available, the Makefile will fall back to regular gcc with 32-bit flags.

### Build Steps

1. Clone the repository:
   ```bash
   git clone https://github.com/lakladon/lakos
   cd lakos
   git pull
   ```

2. Build the kernel:
   ```bash
   make
   ```

This will produce `lakos.bin`, the kernel binary.

## Running

To run Lakos OS, you need to create a bootable ISO image.

### Creating Bootable ISO

1. Install GRUB tools:
   ```bash
   sudo apt-get install grub-pc-bin xorriso
   ```

2. Create ISO directory structure:
   ```bash
   mkdir -p iso/boot/grub
   cp lakos.bin iso/boot/
   ```

3. Create GRUB configuration (`iso/boot/grub/grub.cfg`):
   ```
   set timeout=0
   set default=0

   menuentry "Lakos OS" {
       multiboot /boot/lakos.bin
   }
   ```

4. Generate ISO:
   ```bash
   grub-mkrescue -o lakos.iso iso/
   ```

### Running with QEMU

Install QEMU:
```bash
sudo apt-get install qemu-system-i386
```

Run the OS:
```bash
qemu-system-i386 -cdrom lakos.iso
```

## Project Structure

```
lakos_beta0/
├── boot/
│   └── boot.asm              # Multiboot bootloader
├── kernel/
│   ├── kernel.c              # Main kernel entry point
│   ├── shell.c               # Command-line shell implementation
│   ├── vga.c                 # VGA graphics mode handling
│   ├── gui.c                 # GUI components (if any)
│   ├── idt.c                 # Interrupt Descriptor Table
│   ├── isr.c                 # Interrupt Service Routines
│   ├── gdt.c                 # Global Descriptor Table
│   ├── drivers/
│   │   ├── keyboard.c        # Keyboard driver
│   │   ├── mouse.c           # Mouse driver
│   │   └── io.h              # I/O port utilities
│   ├── fs/
│   │   └── tar.c             # TAR filesystem support (experimental)
│   └── include/
│       ├── gdt.h             # GDT definitions
│       ├── idt.h             # IDT definitions
│       └── multiboot.h       # Multiboot header definitions
├── rootfs/
│   └── bin/                  # User-space programs
│       ├── hello
│       └── test
├── Makefile                  # Build configuration
├── linker.ld                 # Linker script
└── README.md                 # This file
```

## Shell Commands

The built-in shell supports the following commands:

- `help` - Display available commands
- `cls` - Clear screen
- `ver` - Show OS version
- `pwd` - Print current directory
- `ls` - List directory contents
- `cd <dir>` - Change directory
- `echo` - Echo command (placeholder)
- `uname` - Print system name
- `date` - Print current date
- Programs: `hello`, `test`, `editor`

## Contributing

This is a learning project. Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

This project is released under the MIT License. See LICENSE file for details (if applicable).

## Disclaimer

Lakos OS is experimental software and may contain bugs. It is not intended for production use. Use at your own risk.
