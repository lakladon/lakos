/*
 * Lakos OS
 * Copyright (c) 2026 lakladon
 * Disks command - list detected ATA drives
 */

#include <stdint.h>

extern int ata_detect_disks();
extern void terminal_writestring(const char*);
extern void terminal_putchar(char);

static void cmd_disks(const char* args) {
    (void)args;
    terminal_writestring("Detected disks:\n");
    ata_detect_disks();  // This prints detected drives
}
