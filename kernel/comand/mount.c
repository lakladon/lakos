/*
 * Lakos OS
 * Copyright (c) 2026 lakladon
 * Mount command implementation
 */

#include <stdint.h>
#include "include/lib.h"
#include "include/fat32.h"

extern void terminal_writestring(const char* s);

// Parse drive number from string (e.g., "0", "1", "hda", "hdb", "hd0", "hd1")
static int parse_drive(const char* str) {
    if (!str || !str[0]) return -1;
    
    // Skip "hd" prefix if present
    if (str[0] == 'h' && str[1] == 'd') {
        str += 2;
        // Check for letter (hda=0, hdb=1, hdc=2, hdd=3)
        if (str[0] >= 'a' && str[0] <= 'd') {
            return str[0] - 'a';
        }
    }
    
    if (str[0] >= '0' && str[0] <= '9') {
        return str[0] - '0';
    }
    
    return -1;
}

// Parse partition number from string
static uint32_t parse_partition(const char* str) {
    if (!str || !str[0]) return 0;
    
    uint32_t result = 0;
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result;
}

static void cmd_mount(const char* args) {
    if (strlen(args) == 0) {
        terminal_writestring("Usage: mount <drive> [partition] [mount_point]\n");
        terminal_writestring("       mount -l              (list mounted filesystems)\n");
        terminal_writestring("       mount -u <mount_point> (unmount)\n");
        terminal_writestring("\nExamples:\n");
        terminal_writestring("  mount 0              - mount drive 0, partition 0 at /mnt\n");
        terminal_writestring("  mount 0 1 /data      - mount drive 0, partition 1 at /data\n");
        terminal_writestring("  mount hd0 0 /fat32   - mount drive 0, partition 0 at /fat32\n");
        return;
    }
    
    // Parse arguments
    char arg1[64] = {0};
    char arg2[64] = {0};
    char arg3[64] = {0};
    
    int i = 0;
    const char* p = args;
    
    // Skip leading spaces
    while (*p == ' ') p++;
    
    // Get first argument
    while (*p && *p != ' ' && i < 63) {
        arg1[i++] = *p++;
    }
    arg1[i] = '\0';
    
    // Skip spaces
    while (*p == ' ') p++;
    
    // Get second argument
    i = 0;
    while (*p && *p != ' ' && i < 63) {
        arg2[i++] = *p++;
    }
    arg2[i] = '\0';
    
    // Skip spaces
    while (*p == ' ') p++;
    
    // Get third argument
    i = 0;
    while (*p && *p != ' ' && i < 63) {
        arg3[i++] = *p++;
    }
    arg3[i] = '\0';
    
    // Handle -l flag (list mounted filesystems)
    if (strcmp(arg1, "-l") == 0) {
        terminal_writestring("Mounted FAT32 filesystems:\n");
        int found = 0;
        for (int j = 0; j < 4; j++) {
            fat32_fs_t* fs = fat32_get_mounted_fs_by_index(j);
            if (fs && fs->mounted) {
                terminal_writestring("  ");
                terminal_writestring(fs->mount_point);
                terminal_writestring(" - drive ");
                char buf[16];
                itoa(fs->drive, buf);
                terminal_writestring(buf);
                terminal_writestring(", partition at sector ");
                itoa(fs->partition_start, buf);
                terminal_writestring(buf);
                terminal_writestring("\n");
                found = 1;
            }
        }
        if (!found) {
            terminal_writestring("  (none)\n");
        }
        return;
    }
    
    // Handle -u flag (unmount)
    if (strcmp(arg1, "-u") == 0) {
        if (arg2[0] == '\0') {
            terminal_writestring("mount: missing mount point for unmount\n");
            return;
        }
        if (fat32_unmount(arg2) == 0) {
            terminal_writestring("Unmounted ");
            terminal_writestring(arg2);
            terminal_writestring("\n");
        }
        return;
    }
    
    // Parse drive number
    int drive = parse_drive(arg1);
    if (drive < 0) {
        terminal_writestring("mount: invalid drive number\n");
        return;
    }
    
    // Parse partition and mount point
    uint32_t partition = 0;
    const char* mount_point = "/mnt";
    
    if (arg2[0] != '\0') {
        // Check if arg2 is a number (partition) or a path (mount point)
        if (arg2[0] >= '0' && arg2[0] <= '9') {
            partition = parse_partition(arg2);
            if (arg3[0] != '\0') {
                mount_point = arg3;
            }
        } else {
            // arg2 is a mount point
            mount_point = arg2;
        }
    }
    
    // Try to mount as FAT32
    terminal_writestring("Attempting to mount FAT32 filesystem...\n");
    
    if (fat32_mount((uint8_t)drive, partition, mount_point) == 0) {
        terminal_writestring("Successfully mounted at ");
        terminal_writestring(mount_point);
        terminal_writestring("\n");
    }
}