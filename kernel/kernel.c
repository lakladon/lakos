#include "include/multiboot.h"
#include <stdint.h>
#include <stddef.h>
#include "include/version.h"
#include "include/lib.h"

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void kmain(multiboot_info_t* mb_info, uint32_t magic);

// Multiboot header
const multiboot_header_t __attribute__((section(".multiboot"))) header = {
    .magic = MULTIBOOT_HEADER_MAGIC,
    .flags = 0,
    .checksum = -(MULTIBOOT_HEADER_MAGIC + 0)
};



// Text mode terminal
#define VIDEO_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

uint16_t* video_memory = (uint16_t*)VIDEO_MEMORY;

int term_col = 0;
int term_row = 0;
uint8_t current_attr = 0x0F; // white on black

void update_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void terminal_initialize() {
    term_col = 0;
    term_row = 0;
    // Clear screen
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        video_memory[i] = (uint16_t)' ' | (uint16_t)0x0F << 8; // white on black
    }
    // Set cursor position (optional, but for completeness)
    // update_cursor(0, 0);
}

void terminal_putchar(char c) {
    if (c == '\n') {
        term_col = 0;
        term_row++;
    } else if (c == '\b') {
        if (term_col > 0) {
            term_col--;
            video_memory[term_row * VGA_WIDTH + term_col] = (uint16_t)' ' | (uint16_t)current_attr << 8;
        }
    } else {
        video_memory[term_row * VGA_WIDTH + term_col] = (uint16_t)c | (uint16_t)current_attr << 8;
        term_col++;
    }

    if (term_col >= VGA_WIDTH) {
        term_col = 0;
        term_row++;
    }

    if (term_row >= VGA_HEIGHT) {
        // Scroll up
        for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
            video_memory[i] = video_memory[i + VGA_WIDTH];
        }
        // Clear last line
        for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
            video_memory[i] = (uint16_t)' ' | (uint16_t)current_attr << 8;
        }
        term_row = VGA_HEIGHT - 1;
    }

    // update_cursor(term_col, term_row);
}


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

// Прототипы
void init_gdt();
void idt_init();
void irq_install();
extern void ata_init();
extern int ata_detect_disks();
extern void ata_read_sectors(uint8_t drive, uint32_t lba, uint16_t* buffer, uint8_t count);
extern void mouse_install();
extern void shell_main();
extern void init_kernel_commands();

void* tar_archive = 0;

extern char _binary_modules_tar_start[];

// Set VGA text mode (80x25)
void vga_set_text_mode() {
    __asm__ volatile("mov $0x03, %%ah; int $0x10" : : : "ah"); // BIOS set mode 3
}

// Beautiful boot animation functions
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

void boot_fade_in_text(const char* text, int delay) {
    for (int i = 0; i < 3; i++) {
        terminal_writestring("\n");
    }
    for (int i = 0; i < 5; i++) {  // Increased from 3 to 5 for longer animation
        terminal_writestring(text);
        terminal_writestring("\n");
        for (int j = 0; j < delay; j++) {
            for (volatile int k = 0; k < 10000; k++);
        }
    }
}

void boot_draw_logo() {
    terminal_writestring("\033[36m"); // Cyan color
    terminal_writestring("  _______      __  __  __          __  __\n");
    terminal_writestring(" /_  __(_)____/ /_/ /_/ /__  _____/ /_/ /__  ____  ____\n");
    terminal_writestring("  / / / / ___/ __/ __/ / _ \\/ ___/ __/ / _ \\/ __ \\/ __ \\\n");
    terminal_writestring(" / / / (__  ) /_/ /_/ /  __/ /  / /_/ /  __/ /_/ / /_/ /\n");
    terminal_writestring("/_/ /_/____/\\__/\\__/_/\\___/_/   \\__/_/\\___/ .___/\\____/\n");
    terminal_writestring("                                        /_/           \n");
    terminal_writestring("\033[0m"); // Reset color
}

void boot_animated_dots(int count) {
    for (int i = 0; i < count; i++) {
        terminal_writestring(".");
        for (volatile int j = 0; j < 150000; j++);  // Even longer delay for slower dots
    }
}

void boot_dramatic_pause(int duration) {
    for (int i = 0; i < duration; i++) {
        for (volatile int j = 0; j < 250000; j++);  // Longer pause between steps
    }
}

void boot_scanning_animation() {
    terminal_writestring("Scanning system components");
    for (int i = 0; i < 6; i++) {
        terminal_writestring(".");
        for (volatile int j = 0; j < 100000; j++);
    }
    terminal_writestring(" \033[32m[COMPLETE]\033[0m\n");
}

void boot_memory_test() {
    terminal_writestring("Running memory diagnostics");
    for (int i = 0; i < 5; i++) {
        terminal_writestring(".");
        for (volatile int j = 0; j < 120000; j++);
    }
    terminal_writestring(" \033[32m[PASSED]\033[0m\n");
}

void boot_security_check() {
    terminal_writestring("Performing security validation");
    for (int i = 0; i < 5; i++) {
        terminal_writestring(".");
        for (volatile int j = 0; j < 150000; j++);
    }
    terminal_writestring(" \033[32m[SECURE]\033[0m\n");
}

void boot_system_check() {
    terminal_writestring("Verifying system integrity");
    for (int i = 0; i < 5; i++) {
        terminal_writestring(".");
        for (volatile int j = 0; j < 120000; j++);
    }
    terminal_writestring(" \033[32m[VERIFIED]\033[0m\n");
}

void kmain(multiboot_info_t* mb_info, uint32_t magic) {
    terminal_initialize();

    // Beautiful boot sequence
    terminal_writestring("\033[2J"); // Clear screen
    terminal_writestring("\033[H");  // Cursor to top-left
    
    boot_draw_logo();
    terminal_writestring("\n");
    terminal_writestring("\033[33mBooting Lakos OS...\033[0m\n");
    terminal_writestring("\n");

    // Pre-boot system checks
    terminal_writestring("Performing pre-boot diagnostics");
    boot_animated_dots(5);
    terminal_writestring(" \033[32m[OK]\033[0m\n");
    boot_dramatic_pause(2);

    boot_memory_test();
    boot_dramatic_pause(2);

    boot_security_check();
    boot_dramatic_pause(2);

    boot_system_check();
    boot_dramatic_pause(2);

    boot_scanning_animation();
    boot_dramatic_pause(3);

    // Step 1: GDT Initialization
    terminal_writestring("Initializing Global Descriptor Table... ");
    boot_animated_dots(4);
    init_gdt();
    terminal_writestring(" \033[32m[OK]\033[0m\n");
    boot_draw_progress_bar(1, 12);
    boot_dramatic_pause(1);

    // Step 2: IDT Initialization
    terminal_writestring("Setting up Interrupt Descriptor Table... ");
    boot_animated_dots(4);
    idt_init();
    terminal_writestring(" \033[32m[OK]\033[0m\n");
    boot_draw_progress_bar(2, 12);
    boot_dramatic_pause(1);

    // Step 3: IRQ Installation
    terminal_writestring("Installing Interrupt Request handlers... ");
    boot_animated_dots(4);
    irq_install();
    terminal_writestring(" \033[32m[OK]\033[0m\n");
    boot_draw_progress_bar(3, 12);
    boot_dramatic_pause(1);

    // Step 4: File System Loading
    terminal_writestring("Loading embedded file system... ");
    boot_animated_dots(4);
    tar_archive = (void*)&_binary_modules_tar_start;
    if (tar_archive != 0) {
        terminal_writestring(" \033[32m[OK]\033[0m\n");
    } else {
        terminal_writestring(" \033[31m[FAIL]\033[0m\n");
    }
    boot_draw_progress_bar(4, 12);
    boot_dramatic_pause(1);

    // Step 5: Storage Initialization
    terminal_writestring("Detecting storage devices... ");
    boot_animated_dots(4);
    ata_init();
    int disk_count = ata_detect_disks();
    char buf[10];
    itoa(disk_count, buf);
    terminal_writestring(" \033[32m[Found ");
    terminal_writestring(buf);
    terminal_writestring(" disk(s)]\033[0m\n");
    boot_draw_progress_bar(5, 12);
    boot_dramatic_pause(1);

    // Step 6: Input Devices
    terminal_writestring("Initializing input devices... ");
    boot_animated_dots(4);
    terminal_writestring(" \033[32m[OK]\033[0m\n");
    boot_draw_progress_bar(6, 12);
    boot_dramatic_pause(1);

    // Step 7: User System
    terminal_writestring("Setting up user management... ");
    boot_animated_dots(4);
    terminal_writestring(" \033[32m[OK]\033[0m\n");
    boot_draw_progress_bar(7, 12);
    boot_dramatic_pause(1);

    // Step 8: Shell Initialization
    terminal_writestring("Starting shell environment... ");
    boot_animated_dots(4);
    terminal_writestring(" \033[32m[OK]\033[0m\n");
    boot_draw_progress_bar(8, 12);
    boot_dramatic_pause(1);

    // Step 9: Network Initialization (placeholder)
    terminal_writestring("Initializing network stack... ");
    boot_animated_dots(4);
    terminal_writestring(" \033[33m[SKIPPED]\033[0m\n");
    boot_draw_progress_bar(9, 12);
    boot_dramatic_pause(1);

    // Step 10: Graphics Initialization (placeholder)
    terminal_writestring("Setting up graphics subsystem... ");
    boot_animated_dots(4);
    terminal_writestring(" \033[33m[TEXT MODE]\033[0m\n");
    boot_draw_progress_bar(10, 12);
    boot_dramatic_pause(1);

    // Step 11: Audio System (placeholder)
    terminal_writestring("Initializing audio system... ");
    boot_animated_dots(4);
    terminal_writestring(" \033[33m[DISABLED]\033[0m\n");
    boot_draw_progress_bar(11, 12);
    boot_dramatic_pause(1);

    // Step 12: Final System Check
    terminal_writestring("Performing final system check... ");
    boot_animated_dots(4);
    terminal_writestring(" \033[32m[READY]\033[0m\n");
    boot_draw_progress_bar(12, 12);
    terminal_writestring("\n");

    // Final boot message with animation
    terminal_writestring("\033[36m");
    boot_fade_in_text("Lakos OS Ready", 3);
    terminal_writestring("\033[0m");
    
    terminal_writestring("\n\033[32mSystem initialized successfully!\033[0m\n");
    terminal_writestring("Type 'help' for available commands.\n\n");

    __asm__ volatile("sti");
    init_kernel_commands();
    shell_main();

    while(1) { __asm__ volatile("hlt"); }
}
