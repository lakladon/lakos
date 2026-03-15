/*
 * Lakos OS
 * Copyright (c) 2026 lakladon
 * Created: January 11, 2026
 */

#include <stdint.h>
#include "io.h"
#include "include/lib.h"

extern void terminal_writestring(const char*);

void print_hex(uint32_t n, int digits) {
    char buf[9];
    buf[digits] = 0;
    for (int i = digits - 1; i >= 0; i--) {
        buf[i] = "0123456789ABCDEF"[n & 0xF];
        n >>= 4;
    }
    terminal_writestring(buf);
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

// Primary ATA ports
#define ATA_PRIMARY_DATA 0x1F0
#define ATA_PRIMARY_FEATURES 0x1F1
#define ATA_PRIMARY_SECTOR_COUNT 0x1F2
#define ATA_PRIMARY_LBA_LOW 0x1F3
#define ATA_PRIMARY_LBA_MID 0x1F4
#define ATA_PRIMARY_LBA_HIGH 0x1F5
#define ATA_PRIMARY_DRIVE 0x1F6
#define ATA_PRIMARY_COMMAND 0x1F7
#define ATA_PRIMARY_STATUS 0x1F7

// Secondary ATA ports
#define ATA_SECONDARY_DATA 0x170
#define ATA_SECONDARY_FEATURES 0x171
#define ATA_SECONDARY_SECTOR_COUNT 0x172
#define ATA_SECONDARY_LBA_LOW 0x173
#define ATA_SECONDARY_LBA_MID 0x174
#define ATA_SECONDARY_LBA_HIGH 0x175
#define ATA_SECONDARY_DRIVE 0x176
#define ATA_SECONDARY_COMMAND 0x177
#define ATA_SECONDARY_STATUS 0x177

// Commands
#define ATA_CMD_READ 0x20
#define ATA_CMD_WRITE 0x30
#define ATA_CMD_IDENTIFY 0xEC

// Drive types
#define ATA_DRIVE_PRIMARY_MASTER 0
#define ATA_DRIVE_PRIMARY_SLAVE 1
#define ATA_DRIVE_SECONDARY_MASTER 2
#define ATA_DRIVE_SECONDARY_SLAVE 3

// Get base port for drive
static uint16_t ata_get_base(uint8_t drive) {
    if (drive < 2) return ATA_PRIMARY_DATA;
    return ATA_SECONDARY_DATA;
}

static uint16_t ata_get_status_port(uint8_t drive) {
    if (drive < 2) return ATA_PRIMARY_STATUS;
    return ATA_SECONDARY_STATUS;
}

static uint16_t ata_get_drive_port(uint8_t drive) {
    if (drive < 2) return ATA_PRIMARY_DRIVE;
    return ATA_SECONDARY_DRIVE;
}

int ata_wait(uint8_t drive) {
    int timeout = 100000;
    uint16_t status_port = ata_get_status_port(drive);
    while ((inb(status_port) & 0x80) && timeout--); // Wait for BSY to clear with timeout
    return timeout > 0;
}

void ata_select_drive(uint8_t drive) {
    uint16_t drive_port = ata_get_drive_port(drive);
    uint8_t drive_num = (drive & 1); // 0 for master, 1 for slave
    outb(drive_port, 0xE0 | (drive_num << 4));
}

int ata_identify(uint8_t drive) {
    uint16_t base = ata_get_base(drive);
    uint16_t status_port = ata_get_status_port(drive);
    
    ata_select_drive(drive);
    outb(base + 2, 0);  // Sector count
    outb(base + 3, 0);  // LBA low
    outb(base + 4, 0);  // LBA mid
    outb(base + 5, 0);  // LBA high
    outb(base + 7, ATA_CMD_IDENTIFY);

    uint8_t status = inb(status_port);
    if (status == 0) {
        return 0; // No drive
    }

    if (!ata_wait(drive)) {
        return 0; // Timeout
    }
    status = inb(status_port);
    if (status & 0x01) {
        return 0; // ERR bit set
    }

    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(base);
    }

    terminal_writestring("ATA Drive ");
    char buf[4];
    buf[0] = 'h';
    buf[1] = 'd';
    buf[2] = 'a' + drive;
    buf[3] = 0;
    terminal_writestring(buf);
    terminal_writestring(" ID: 0x");
    print_hex(identify_data[0], 4);
    terminal_writestring("\n");

    return 1;
}

void ata_read_sector(uint8_t drive, uint32_t lba, uint16_t* buffer) {
    if (drive > 3) return;
    if (lba > 0xFFFFFF) return; // LBA28 limit
    
    uint16_t base = ata_get_base(drive);
    
    ata_select_drive(drive);
    outb(base + 2, 1);  // Sector count
    outb(base + 3, lba & 0xFF);
    outb(base + 4, (lba >> 8) & 0xFF);
    outb(base + 5, (lba >> 16) & 0xFF);
    outb(base + 7, ATA_CMD_READ);

    if (!ata_wait(drive)) {
        return; // Timeout
    }
    uint16_t status_port = ata_get_status_port(drive);
    uint8_t status = inb(status_port);
    if (status & 0x01) {
        return; // ERR bit set
    }
    
    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(base);
    }
}

void ata_write_sector(uint8_t drive, uint32_t lba, uint16_t* buffer) {
    if (drive > 3) return;
    if (lba > 0xFFFFFF) return; // LBA28 limit
    
    uint16_t base = ata_get_base(drive);
    
    ata_select_drive(drive);
    outb(base + 2, 1);  // Sector count
    outb(base + 3, lba & 0xFF);
    outb(base + 4, (lba >> 8) & 0xFF);
    outb(base + 5, (lba >> 16) & 0xFF);
    outb(base + 7, ATA_CMD_WRITE);

    if (!ata_wait(drive)) return; // Timeout
    for (int i = 0; i < 256; i++) {
        outw(base, buffer[i]);
    }
    ata_wait(drive); // Wait for write to complete
}

void ata_read_sectors(uint8_t drive, uint32_t lba, uint16_t* buffer, uint8_t count) {
    if (drive > 3) return;
    
    uint16_t base = ata_get_base(drive);
    
    ata_select_drive(drive);
    outb(base + 2, count);
    outb(base + 3, lba & 0xFF);
    outb(base + 4, (lba >> 8) & 0xFF);
    outb(base + 5, (lba >> 16) & 0xFF);
    outb(base + 7, ATA_CMD_READ);

    if (!ata_wait(drive)) return; // Timeout
    for (int s = 0; s < count; s++) {
        for (int i = 0; i < 256; i++) {
            buffer[s * 256 + i] = inw(base);
        }
    }
}

void ata_write_sectors(uint8_t drive, uint32_t lba, uint16_t* buffer, uint8_t count) {
    if (drive > 3) return;
    
    uint16_t base = ata_get_base(drive);
    
    ata_select_drive(drive);
    outb(base + 2, count);
    outb(base + 3, lba & 0xFF);
    outb(base + 4, (lba >> 8) & 0xFF);
    outb(base + 5, (lba >> 16) & 0xFF);
    outb(base + 7, ATA_CMD_WRITE);

    if (!ata_wait(drive)) return; // Timeout
    for (int s = 0; s < count; s++) {
        for (int i = 0; i < 256; i++) {
            outw(base, buffer[s * 256 + i]);
        }
    }
}

void ata_init() {
    // Initialize ATA
}

int ata_detect_disks() {
    int count = 0;
    if (ata_identify(0)) count++;  // Primary master
    if (ata_identify(1)) count++;  // Primary slave
    if (ata_identify(2)) count++;  // Secondary master
    if (ata_identify(3)) count++;  // Secondary slave
    return count;
}