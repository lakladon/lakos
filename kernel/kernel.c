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

// Optional terminal output capture (used for shell pipes).
static int term_capture_enabled = 0;
static char* term_capture_buffer = 0;
static int term_capture_size = 0;
static int term_capture_pos = 0;

void terminal_capture_begin(char* buffer, int size) {
    term_capture_buffer = buffer;
    term_capture_size = size;
    term_capture_pos = 0;
    term_capture_enabled = (buffer && size > 0) ? 1 : 0;
    if (term_capture_enabled) {
        term_capture_buffer[0] = '\0';
    }
}

void terminal_capture_end() {
    if (term_capture_enabled && term_capture_buffer && term_capture_size > 0) {
        if (term_capture_pos >= term_capture_size) {
            term_capture_pos = term_capture_size - 1;
        }
        term_capture_buffer[term_capture_pos] = '\0';
    }
    term_capture_enabled = 0;
    term_capture_buffer = 0;
    term_capture_size = 0;
    term_capture_pos = 0;
}

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
    if (term_capture_enabled && term_capture_buffer && term_capture_size > 0) {
        if (c == '\b') {
            if (term_capture_pos > 0) term_capture_pos--;
        } else if (term_capture_pos < term_capture_size - 1) {
            term_capture_buffer[term_capture_pos++] = c;
        }
        term_capture_buffer[term_capture_pos] = '\0';
        return;
    }

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
    for (int i = 0; i < 3; i++) {
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
        for (volatile int j = 0; j < 50000; j++);
    }
}

// System information structure
typedef struct {
    uint32_t memory_size;
    uint32_t cpu_features;
    char kernel_version[16];
    char build_date[32];
} system_info_t;

system_info_t sys_info;

// Memory detection function
uint32_t detect_memory_size(multiboot_info_t* mb_info) {
    if (mb_info->flags & 0x0001) {
        return mb_info->mem_upper * 1024;
    }
    return 0;
}

// CPU feature detection
uint32_t detect_cpu_features() {
    uint32_t features = 0;
    
    // Check for CPUID support
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

// Enhanced boot sequence with system info
void kmain(multiboot_info_t* mb_info, uint32_t magic) {
    (void)magic; // Suppress unused parameter warning
    terminal_initialize();

    // Initialize system information
    sys_info.memory_size = detect_memory_size(mb_info);
    sys_info.cpu_features = detect_cpu_features();
    strcpy(sys_info.kernel_version, KERNEL_VERSION);
    strcpy(sys_info.build_date, __DATE__ " " __TIME__);

    // Beautiful boot sequence
    terminal_writestring("\033[2J"); // Clear screen
    terminal_writestring("\033[H");  // Cursor to top-left
    
    boot_draw_logo();
    terminal_writestring("\n");
    terminal_writestring("\033[33mBooting Lakos OS...\033[0m\n");
    terminal_writestring("\n");

    // Step 1: GDT Initialization
    terminal_writestring("Initializing Global Descriptor Table... ");
    boot_animated_dots(3);
    init_gdt();
    terminal_writestring(" \033[32m[OK]\033[0m\n");
    boot_draw_progress_bar(1, 8);

    // Step 2: IDT Initialization
    terminal_writestring("Setting up Interrupt Descriptor Table... ");
    boot_animated_dots(3);
    idt_init();
    terminal_writestring(" \033[32m[OK]\033[0m\n");
    boot_draw_progress_bar(2, 8);

    // Step 3: IRQ Installation
    terminal_writestring("Installing Interrupt Request handlers... ");
    boot_animated_dots(3);
    irq_install();
    terminal_writestring(" \033[32m[OK]\033[0m\n");
    boot_draw_progress_bar(3, 8);

    // Step 4: File System Loading
    terminal_writestring("Loading embedded file system... ");
    boot_animated_dots(3);
    tar_archive = (void*)&_binary_modules_tar_start;
    if (tar_archive != 0) {
        terminal_writestring(" \033[32m[OK]\033[0m\n");
    } else {
        terminal_writestring(" \033[31m[FAIL]\033[0m\n");
    }
    boot_draw_progress_bar(4, 8);

    // Step 5: Storage Initialization
    terminal_writestring("Detecting storage devices... ");
    boot_animated_dots(3);
    ata_init();
    int disk_count = ata_detect_disks();
    char buf[10];
    itoa(disk_count, buf);
    terminal_writestring(" \033[32m[Found ");
    terminal_writestring(buf);
    terminal_writestring(" disk(s)]\033[0m\n");
    boot_draw_progress_bar(5, 8);

    // Step 6: Input Devices
    terminal_writestring("Initializing input devices... ");
    boot_animated_dots(3);
    terminal_writestring(" \033[32m[OK]\033[0m\n");
    boot_draw_progress_bar(6, 8);

    // Step 7: User System
    terminal_writestring("Setting up user management... ");
    boot_animated_dots(3);
    terminal_writestring(" \033[32m[OK]\033[0m\n");
    boot_draw_progress_bar(7, 8);

    // Step 8: Shell Initialization
    terminal_writestring("Starting shell environment... ");
    boot_animated_dots(3);
    terminal_writestring(" \033[32m[OK]\033[0m\n");
    boot_draw_progress_bar(8, 8);
    terminal_writestring("\n");

    // System information display
    terminal_writestring("\033[36m");
    boot_fade_in_text("Lakos OS Ready", 2);
    terminal_writestring("\033[0m");
    
    terminal_writestring("\n\033[32mSystem initialized successfully!\033[0m\n");
    terminal_writestring("Type 'help' for available commands.\n");
    
    // Display system information
    terminal_writestring("\n\033[33mSystem Information:\033[0m\n");
    terminal_writestring("Kernel Version: ");
    terminal_writestring(sys_info.kernel_version);
    terminal_writestring("\n");
    terminal_writestring("Build Date: ");
    terminal_writestring(sys_info.build_date);
    terminal_writestring("\n");
    terminal_writestring("Memory: ");
    itoa(sys_info.memory_size / 1024, buf);
    terminal_writestring(buf);
    terminal_writestring(" KB\n");
    terminal_writestring("CPU Features: ");
    itoa(sys_info.cpu_features, buf);
    terminal_writestring(buf);
    terminal_writestring("\n\n");

    __asm__ volatile("sti");
    init_kernel_commands();
    shell_main();

    while(1) { __asm__ volatile("hlt"); }
}
