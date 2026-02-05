#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include <stddef.h>

// FAT32 Boot Sector Structure
typedef struct {
    uint8_t jmp[3];                 // Jump instruction
    char oem[8];                    // OEM name
    uint16_t bytes_per_sector;      // Typically 512
    uint8_t sectors_per_cluster;    // Number of sectors in a cluster
    uint16_t reserved_sectors;      // Reserved sectors before FAT
    uint8_t num_fats;               // Number of FATs
    uint16_t root_entries;          // Number of root directory entries (0 for FAT32)
    uint16_t total_sectors_16;      // Total sectors (0 for FAT32)
    uint8_t media;                  // Media descriptor
    uint16_t sectors_per_fat_16;    // Sectors per FAT (0 for FAT32)
    uint16_t sectors_per_track;     // Sectors per track
    uint16_t heads;                 // Number of heads
    uint32_t hidden_sectors;        // Hidden sectors
    uint32_t total_sectors;         // Total sectors (FAT32)
    uint32_t sectors_per_fat;       // Sectors per FAT (FAT32)
    uint16_t flags;                 // Flags
    uint16_t version;               // Version
    uint32_t root_cluster;          // Root directory cluster
    uint16_t fsinfo_sector;         // FSInfo sector
    uint16_t backup_boot_sector;    // Backup boot sector
    uint8_t reserved[12];           // Reserved
    uint8_t drive_number;           // Drive number
    uint8_t reserved_nt;            // Reserved for Windows NT
    uint8_t boot_signature;         // Boot signature
    uint32_t volume_id;             // Volume ID
    char volume_label[11];          // Volume label
    char filesystem_type[8];        // Filesystem type
} __attribute__((packed)) fat32_boot_sector_t;

// FAT32 Directory Entry Structure
typedef struct {
    char filename[8];               // Filename
    char extension[3];              // Extension
    uint8_t attributes;             // File attributes
    uint8_t nt_reserved;            // Reserved for Windows NT
    uint8_t creation_time_tenths;   // Creation time (tenths of second)
    uint16_t creation_time;         // Creation time (HH:MM:SS)
    uint16_t creation_date;         // Creation date (YYYY:MM:DD)
    uint16_t last_access_date;      // Last access date
    uint16_t high_cluster;          // High word of first cluster
    uint16_t write_time;            // Write time
    uint16_t write_date;            // Write date
    uint16_t low_cluster;           // Low word of first cluster
    uint32_t file_size;             // File size in bytes
} __attribute__((packed)) fat32_dir_entry_t;

// Long Filename Entry Structure (LFN)
typedef struct {
    uint8_t order;                  // Order of LFN entry
    uint16_t name1[5];              // First 5 characters
    uint8_t attributes;             // Attributes (always 0x0F for LFN)
    uint8_t type;                   // Type (0 for LFN)
    uint8_t checksum;               // Checksum of short filename
    uint16_t name2[6];              // Next 6 characters
    uint16_t zero;                  // Always 0
    uint16_t name3[2];              // Final 2 characters
} __attribute__((packed)) fat32_lfn_entry_t;

// FAT32 Context
typedef struct {
    void* disk_buffer;              // Raw disk image buffer
    uint32_t bytes_per_sector;
    uint32_t sectors_per_cluster;
    uint32_t cluster_size;
    uint32_t fat_start_sector;
    uint32_t data_start_sector;
    uint32_t root_cluster;
} fat32_context_t;

// Function declarations
void* fat32_init(void* disk_image);
int fat32_check_path_exists(void* context, const char* path);
void* fat32_lookup(void* context, const char* filename);
int fat32_get_file_size(void* context, const char* filename);
void fat32_list_directory(void* context, const char* dirpath);
void fat32_list_files(void* context);
void fat32_get_directories(void* context, char directories[][256], int* count);

#endif // FAT32_H
