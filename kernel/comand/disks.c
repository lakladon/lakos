#include <stdint.h>
extern int ata_detect_disks();
extern void terminal_writestring(const char*);
extern void terminal_putchar(char);
static void cmd_disks(const char* args) {
    (void)args;
    terminal_writestring("Detected disks:\n");
    ata_detect_disks();  
}
