#include <stdint.h>
#include "io.h"

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

// ATA ports
#define ATA_DATA 0x1F0
#define ATA_FEATURES 0x1F1
#define ATA_SECTOR_COUNT 0x1F2
#define ATA_LBA_LOW 0x1F3
#define ATA_LBA_MID 0x1F4
#define ATA_LBA_HIGH 0x1F5
#define ATA_DRIVE 0x1F6
#define ATA_COMMAND 0x1F7
#define ATA_STATUS 0x1F7

// Commands
#define ATA_CMD_READ 0x20
#define ATA_CMD_WRITE 0x30
#define ATA_CMD_IDENTIFY 0xEC

int ata_wait() {
    int timeout = 100000;
    while ((inb(ATA_STATUS) & 0x80) && timeout--); // Wait for BSY to clear with timeout
    return timeout > 0;
}

void ata_select_drive(uint8_t drive) {
    outb(ATA_DRIVE, 0xE0 | (drive << 4));
}

int ata_identify(uint8_t drive) {
    ata_select_drive(drive);
    outb(ATA_SECTOR_COUNT, 0);
    outb(ATA_LBA_LOW, 0);
    outb(ATA_LBA_MID, 0);
    outb(ATA_LBA_HIGH, 0);
    outb(ATA_COMMAND, ATA_CMD_IDENTIFY);

    uint8_t status = inb(ATA_STATUS);
    if (status == 0) return 0; // No drive

    if (!ata_wait()) return 0; // Timeout
    status = inb(ATA_STATUS);
    if (status & 0x01) return 0; // ERR bit set
    // Read identify data (not implemented fully)
    return 1;
}

void ata_read_sector(uint8_t drive, uint32_t lba, uint16_t* buffer) {
    if (drive > 1) return;
    if (lba > 0xFFFFFF) return; // LBA28 limit
    ata_select_drive(drive);
    outb(ATA_SECTOR_COUNT, 1);
    outb(ATA_LBA_LOW, lba & 0xFF);
    outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(ATA_COMMAND, ATA_CMD_READ);

    if (!ata_wait()) return; // Timeout, don't read
    uint8_t status = inb(ATA_STATUS);
    if (status & 0x01) return; // ERR bit set
    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(ATA_DATA);
    }
}

void ata_write_sector(uint8_t drive, uint32_t lba, uint16_t* buffer) {
    if (drive > 1) return;
    if (lba > 0xFFFFFF) return; // LBA28 limit
    ata_select_drive(drive);
    outb(ATA_SECTOR_COUNT, 1);
    outb(ATA_LBA_LOW, lba & 0xFF);
    outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(ATA_COMMAND, ATA_CMD_WRITE);

    if (!ata_wait()) return; // Timeout, don't write
    for (int i = 0; i < 256; i++) {
        outw(ATA_DATA, buffer[i]);
    }
    // Flush cache (simplified)
    ata_wait(); // Wait for write to complete
}

void ata_read_sectors(uint8_t drive, uint32_t lba, uint16_t* buffer, uint8_t count) {
    ata_select_drive(drive);
    outb(ATA_SECTOR_COUNT, count);
    outb(ATA_LBA_LOW, lba & 0xFF);
    outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(ATA_COMMAND, ATA_CMD_READ);

    if (!ata_wait()) return; // Timeout
    for (int s = 0; s < count; s++) {
        for (int i = 0; i < 256; i++) {
            buffer[s * 256 + i] = inw(ATA_DATA);
        }
    }
}

void ata_write_sectors(uint8_t drive, uint32_t lba, uint16_t* buffer, uint8_t count) {
    ata_select_drive(drive);
    outb(ATA_SECTOR_COUNT, count);
    outb(ATA_LBA_LOW, lba & 0xFF);
    outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(ATA_COMMAND, ATA_CMD_WRITE);

    if (!ata_wait()) return; // Timeout
    for (int s = 0; s < count; s++) {
        for (int i = 0; i < 256; i++) {
            outw(ATA_DATA, buffer[s * 256 + i]);
        }
    }
    // Flush cache
}

void ata_init() {
    // Initialize ATA
}

int ata_detect_disks() {
    int count = 0;
    if (ata_identify(0)) count++;
    if (ata_identify(1)) count++;
    return count;
}