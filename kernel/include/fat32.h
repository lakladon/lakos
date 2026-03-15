/*
 * Lakos OS
 * Copyright (c) 2026 lakladon
 * FAT32 Filesystem Driver
 */

#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>

// FAT32 Boot Sector Structure
typedef struct {
    uint8_t  jmp_boot[3];
    uint8_t  oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t  num_fats;
    uint16_t root_entry_count;      // 0 for FAT32
    uint16_t total_sectors_16;      // 0 for FAT32
    uint8_t  media_type;
    uint16_t fat_size_16;           // 0 for FAT32
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    
    // FAT32 extended boot record
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_num;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    uint8_t  volume_label[11];
    uint8_t  fs_type[8];
} __attribute__((packed)) fat32_boot_sector_t;

// FAT32 Directory Entry Structure
typedef struct {
    uint8_t  name[11];
    uint8_t  attr;
    uint8_t  reserved;
    uint8_t  create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t last_access_date;
    uint16_t cluster_high;
    uint16_t modify_time;
    uint16_t modify_date;
    uint16_t cluster_low;
    uint32_t file_size;
} __attribute__((packed)) fat32_dir_entry_t;

// FAT32 Long Filename Entry
typedef struct {
    uint8_t  order;
    uint16_t name1[5];
    uint8_t  attr;
    uint8_t  type;
    uint8_t  checksum;
    uint16_t name2[6];
    uint16_t reserved;
    uint16_t name3[2];
} __attribute__((packed)) fat32_lfn_entry_t;

// File attributes
#define FAT32_ATTR_READ_ONLY    0x01
#define FAT32_ATTR_HIDDEN       0x02
#define FAT32_ATTR_SYSTEM       0x04
#define FAT32_ATTR_VOLUME_ID    0x08
#define FAT32_ATTR_DIRECTORY    0x10
#define FAT32_ATTR_ARCHIVE      0x20
#define FAT32_ATTR_LONG_NAME    0x0F

// FAT32 special cluster values
#define FAT32_FREE_CLUSTER      0x00000000
#define FAT32_RESERVED_CLUSTER  0x0FFFFFF0
#define FAT32_BAD_CLUSTER       0x0FFFFFF7
#define FAT32_END_OF_CHAIN      0x0FFFFFF8
#define FAT32_END_OF_CHAIN2     0x0FFFFFFF

// Maximum path length
#define FAT32_MAX_PATH          256
#define FAT32_MAX_FILENAME      13

// FAT32 mounted filesystem structure
typedef struct {
    uint8_t drive;
    uint32_t partition_start;
    uint32_t fat_start;
    uint32_t data_start;
    uint32_t root_cluster;
    uint32_t bytes_per_sector;
    uint32_t sectors_per_cluster;
    uint32_t bytes_per_cluster;
    uint32_t fat_size;
    uint32_t total_clusters;
    uint16_t reserved_sector_count;
    uint8_t num_fats;
    uint8_t mounted;
    char mount_point[64];
} fat32_fs_t;

// File handle structure
typedef struct {
    fat32_fs_t* fs;
    uint32_t first_cluster;
    uint32_t current_cluster;
    uint32_t current_offset;
    uint32_t file_size;
    uint8_t is_directory;
    char name[FAT32_MAX_FILENAME];
} fat32_file_t;

// Function declarations

// Initialize FAT32 subsystem
void fat32_init(void);

// Mount a FAT32 partition
int fat32_mount(uint8_t drive, uint32_t partition_start, const char* mount_point);

// Unmount a FAT32 partition
int fat32_unmount(const char* mount_point);

// Open a file or directory
int fat32_open(fat32_file_t* file, const char* path);

// Read from a file
int fat32_read(fat32_file_t* file, void* buffer, uint32_t size);

// Write to a file
int fat32_write(fat32_file_t* file, const void* buffer, uint32_t size);

// Close a file
void fat32_close(fat32_file_t* file);

// List directory contents
int fat32_list_directory(fat32_fs_t* fs, const char* path);

// Check if path exists
int fat32_exists(fat32_fs_t* fs, const char* path);

// Create a file
int fat32_create(fat32_fs_t* fs, const char* path);

// Create a directory
int fat32_mkdir(fat32_fs_t* fs, const char* path);

// Delete a file
int fat32_delete(fat32_fs_t* fs, const char* path);

// Get file size
int fat32_get_file_size(fat32_fs_t* fs, const char* path);

// Get mounted filesystem
fat32_fs_t* fat32_get_mounted_fs(const char* mount_point);

// Get mounted filesystem by index
fat32_fs_t* fat32_get_mounted_fs_by_index(int index);

// Read cluster chain
uint32_t fat32_read_cluster_chain(fat32_fs_t* fs, uint32_t start_cluster, 
                                   uint8_t* buffer, uint32_t max_bytes);

// Get next cluster in chain
uint32_t fat32_get_next_cluster(fat32_fs_t* fs, uint32_t cluster);

// Find file in directory
int fat32_find_entry(fat32_fs_t* fs, uint32_t dir_cluster, const char* name, 
                     fat32_dir_entry_t* entry);

// Parse path into components
int fat32_parse_path(const char* path, char components[][FAT32_MAX_FILENAME], int max_components);

#endif // FAT32_H