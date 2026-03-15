/*
 * Lakos OS
 * Copyright (c) 2026 lakladon
 * FAT32 Filesystem Driver Implementation
 */

#include <stdint.h>
#include <stddef.h>
#include "include/lib.h"
#include "include/fat32.h"
#include "drivers/io.h"

extern void terminal_writestring(const char* s);
extern void ata_read_sector(uint8_t drive, uint32_t lba, uint16_t* buffer);
extern void ata_write_sector(uint8_t drive, uint32_t lba, uint16_t* buffer);

// Maximum number of mounted FAT32 filesystems
#define MAX_FAT32_MOUNTS 4

// Mounted filesystems
static fat32_fs_t mounted_fs[MAX_FAT32_MOUNTS];

// Sector buffer for reading
static uint16_t sector_buffer[256];
static uint8_t cluster_buffer[4096];  // Max cluster size 4KB

// Initialize FAT32 subsystem
void fat32_init(void) {
    for (int i = 0; i < MAX_FAT32_MOUNTS; i++) {
        mounted_fs[i].mounted = 0;
        mounted_fs[i].mount_point[0] = '\0';
    }
}

// Get mounted filesystem by mount point
fat32_fs_t* fat32_get_mounted_fs(const char* mount_point) {
    for (int i = 0; i < MAX_FAT32_MOUNTS; i++) {
        if (mounted_fs[i].mounted && 
            strcmp(mounted_fs[i].mount_point, mount_point) == 0) {
            return &mounted_fs[i];
        }
    }
    return NULL;
}

// Get mounted filesystem by index
fat32_fs_t* fat32_get_mounted_fs_by_index(int index) {
    if (index < 0 || index >= MAX_FAT32_MOUNTS) {
        return NULL;
    }
    return &mounted_fs[index];
}

// Read a sector from the drive
static int fat32_read_sector(fat32_fs_t* fs, uint32_t sector, uint8_t* buffer) {
    uint32_t lba = fs->partition_start + sector;
    ata_read_sector(fs->drive, lba, (uint16_t*)sector_buffer);
    memcpy(buffer, sector_buffer, 512);
    return 0;
}

// Write a sector to the drive
static int fat32_write_sector(fat32_fs_t* fs, uint32_t sector, const uint8_t* buffer) {
    uint32_t lba = fs->partition_start + sector;
    memcpy(sector_buffer, buffer, 512);
    ata_write_sector(fs->drive, lba, sector_buffer);
    return 0;
}

// Read a cluster from the filesystem
static int fat32_read_cluster(fat32_fs_t* fs, uint32_t cluster, uint8_t* buffer) {
    uint32_t first_sector = fs->data_start + 
                            (cluster - 2) * fs->sectors_per_cluster;
    
    for (uint32_t i = 0; i < fs->sectors_per_cluster; i++) {
        if (fat32_read_sector(fs, first_sector + i, 
                              buffer + i * fs->bytes_per_sector) != 0) {
            return -1;
        }
    }
    return 0;
}

// Write a cluster to the filesystem
static int fat32_write_cluster(fat32_fs_t* fs, uint32_t cluster, const uint8_t* buffer) {
    uint32_t first_sector = fs->data_start + 
                            (cluster - 2) * fs->sectors_per_cluster;
    
    for (uint32_t i = 0; i < fs->sectors_per_cluster; i++) {
        if (fat32_write_sector(fs, first_sector + i, 
                               buffer + i * fs->bytes_per_sector) != 0) {
            return -1;
        }
    }
    return 0;
}

// Get the next cluster in the chain from the FAT
uint32_t fat32_get_next_cluster(fat32_fs_t* fs, uint32_t cluster) {
    if (cluster < 2 || cluster >= fs->total_clusters + 2) {
        return FAT32_BAD_CLUSTER;
    }
    
    // Each FAT entry is 32 bits (4 bytes)
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fs->fat_start + fat_offset / fs->bytes_per_sector;
    uint32_t fat_offset_in_sector = fat_offset % fs->bytes_per_sector;
    
    // Read the FAT sector
    uint8_t fat_sector_data[512];
    if (fat32_read_sector(fs, fat_sector, fat_sector_data) != 0) {
        return FAT32_BAD_CLUSTER;
    }
    
    // Extract the 32-bit FAT entry (lower 28 bits are used)
    uint32_t fat_entry = *((uint32_t*)(fat_sector_data + fat_offset_in_sector));
    fat_entry &= 0x0FFFFFFF;  // Lower 28 bits are valid
    
    return fat_entry;
}

// Set a FAT entry
static int fat32_set_fat_entry(fat32_fs_t* fs, uint32_t cluster, uint32_t value) {
    if (cluster < 2 || cluster >= fs->total_clusters + 2) {
        return -1;
    }
    
    value &= 0x0FFFFFFF;  // Lower 28 bits
    
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fs->fat_start + fat_offset / fs->bytes_per_sector;
    uint32_t fat_offset_in_sector = fat_offset % fs->bytes_per_sector;
    
    // Read the FAT sector
    uint8_t fat_sector_data[512];
    if (fat32_read_sector(fs, fat_sector, fat_sector_data) != 0) {
        return -1;
    }
    
    // Modify the FAT entry
    uint32_t* entry = (uint32_t*)(fat_sector_data + fat_offset_in_sector);
    *entry = (*entry & 0xF0000000) | value;
    
    // Write back the FAT sector
    if (fat32_write_sector(fs, fat_sector, fat_sector_data) != 0) {
        return -1;
    }
    
    return 0;
}

// Find a free cluster
static uint32_t fat32_find_free_cluster(fat32_fs_t* fs) {
    for (uint32_t cluster = 2; cluster < fs->total_clusters + 2; cluster++) {
        if (fat32_get_next_cluster(fs, cluster) == FAT32_FREE_CLUSTER) {
            return cluster;
        }
    }
    return 0;  // No free cluster found
}

// Mount a FAT32 partition
int fat32_mount(uint8_t drive, uint32_t partition_start, const char* mount_point) {
    // Find a free mount slot
    int slot = -1;
    for (int i = 0; i < MAX_FAT32_MOUNTS; i++) {
        if (!mounted_fs[i].mounted) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) {
        terminal_writestring("FAT32: No free mount slots\n");
        return -1;
    }
    
    // Read the boot sector
    fat32_boot_sector_t boot_sector;
    ata_read_sector(drive, partition_start, (uint16_t*)sector_buffer);
    memcpy(&boot_sector, sector_buffer, sizeof(fat32_boot_sector_t));
    
    // Verify FAT32 signature
    if (boot_sector.boot_signature != 0x29 && boot_sector.boot_signature != 0x28) {
        terminal_writestring("FAT32: Invalid boot signature\n");
        return -1;
    }
    
    // Check if it's actually FAT32 (FAT16 has non-zero fat_size_16)
    if (boot_sector.fat_size_16 != 0) {
        terminal_writestring("FAT32: Not a FAT32 filesystem (FAT12/16 detected)\n");
        return -1;
    }
    
    // Verify "FAT32" string in fs_type
    if (strncmp((char*)boot_sector.fs_type, "FAT32", 5) != 0) {
        terminal_writestring("FAT32: FS type mismatch\n");
        // Continue anyway, some implementations don't set this correctly
    }
    
    // Initialize the filesystem structure
    fat32_fs_t* fs = &mounted_fs[slot];
    fs->drive = drive;
    fs->partition_start = partition_start;
    fs->bytes_per_sector = boot_sector.bytes_per_sector;
    fs->sectors_per_cluster = boot_sector.sectors_per_cluster;
    fs->bytes_per_cluster = fs->bytes_per_sector * fs->sectors_per_cluster;
    fs->reserved_sector_count = boot_sector.reserved_sector_count;
    fs->fat_size = boot_sector.fat_size_32;
    fs->num_fats = boot_sector.num_fats;
    fs->root_cluster = boot_sector.root_cluster;
    
    // Calculate FAT start
    fs->fat_start = fs->reserved_sector_count;
    
    // Calculate data start
    fs->data_start = fs->reserved_sector_count + 
                     (fs->num_fats * fs->fat_size);
    
    // Calculate total clusters
    uint32_t total_sectors = boot_sector.total_sectors_16 ? 
                             boot_sector.total_sectors_16 : 
                             boot_sector.total_sectors_32;
    uint32_t data_sectors = total_sectors - fs->data_start;
    fs->total_clusters = data_sectors / fs->sectors_per_cluster;
    
    // Set mount point
    strncpy(fs->mount_point, mount_point, 63);
    fs->mount_point[63] = '\0';
    fs->mounted = 1;
    
    // Print info
    terminal_writestring("FAT32: Mounted ");
    terminal_writestring(mount_point);
    terminal_writestring(" on drive ");
    char buf[16];
    itoa(drive, buf);
    terminal_writestring(buf);
    terminal_writestring(", partition ");
    itoa(partition_start, buf);
    terminal_writestring(buf);
    terminal_writestring("\n  Bytes per sector: ");
    itoa(fs->bytes_per_sector, buf);
    terminal_writestring(buf);
    terminal_writestring("\n  Sectors per cluster: ");
    itoa(fs->sectors_per_cluster, buf);
    terminal_writestring(buf);
    terminal_writestring("\n  Total clusters: ");
    itoa(fs->total_clusters, buf);
    terminal_writestring(buf);
    terminal_writestring("\n  Volume label: ");
    char label[12];
    strncpy(label, (char*)boot_sector.volume_label, 11);
    label[11] = '\0';
    terminal_writestring(label);
    terminal_writestring("\n");
    
    return 0;
}

// Unmount a FAT32 partition
int fat32_unmount(const char* mount_point) {
    fat32_fs_t* fs = fat32_get_mounted_fs(mount_point);
    if (!fs) {
        terminal_writestring("FAT32: Mount point not found\n");
        return -1;
    }
    
    fs->mounted = 0;
    fs->mount_point[0] = '\0';
    
    terminal_writestring("FAT32: Unmounted ");
    terminal_writestring(mount_point);
    terminal_writestring("\n");
    
    return 0;
}

// Convert 8.3 filename to normal string
static void fat32_name_to_string(const uint8_t* fat_name, char* out) {
    int i = 0;
    int j = 0;
    
    // Copy the name part (8 characters)
    for (i = 0; i < 8 && fat_name[i] != ' '; i++) {
        out[j++] = fat_name[i];
    }
    
    // Check if there's an extension
    if (fat_name[8] != ' ') {
        out[j++] = '.';
        for (i = 8; i < 11 && fat_name[i] != ' '; i++) {
            out[j++] = fat_name[i];
        }
    }
    
    out[j] = '\0';
}

// Convert string to 8.3 filename format
static void string_to_fat_name(const char* name, uint8_t* fat_name) {
    int i = 0;
    int j = 0;
    
    // Initialize with spaces
    memset(fat_name, ' ', 11);
    
    // Copy name part (up to 8 characters)
    while (name[i] && name[i] != '.' && j < 8) {
        fat_name[j++] = name[i++] | 0x40;  // Convert to uppercase
    }
    
    // Skip extension separator
    if (name[i] == '.') {
        i++;
        j = 8;
        // Copy extension (up to 3 characters)
        while (name[i] && j < 11) {
            fat_name[j++] = name[i++] | 0x40;  // Convert to uppercase
        }
    }
}

// Compare FAT 8.3 name with a regular name
static int fat32_compare_name(const uint8_t* fat_name, const char* name) {
    char fat_str[13];
    fat32_name_to_string(fat_name, fat_str);
    
    // Case-insensitive comparison
    for (int i = 0; fat_str[i] && name[i]; i++) {
        char c1 = fat_str[i] | 0x20;  // Convert to lowercase
        char c2 = name[i] | 0x20;
        if (c1 != c2) return 0;
    }
    return 1;
}

// Parse path into components
int fat32_parse_path(const char* path, char components[][FAT32_MAX_FILENAME], int max_components) {
    if (!path || !path[0]) return 0;
    
    // Skip leading slash
    if (path[0] == '/') path++;
    
    int count = 0;
    int pos = 0;
    
    while (path[pos] && count < max_components) {
        int comp_len = 0;
        while (path[pos] && path[pos] != '/' && comp_len < FAT32_MAX_FILENAME - 1) {
            components[count][comp_len++] = path[pos++];
        }
        components[count][comp_len] = '\0';
        
        if (comp_len > 0) {
            count++;
        }
        
        if (path[pos] == '/') pos++;
    }
    
    return count;
}

// Find entry in directory
int fat32_find_entry(fat32_fs_t* fs, uint32_t dir_cluster, const char* name, 
                     fat32_dir_entry_t* entry) {
    uint8_t cluster_data[4096];  // Max cluster size
    uint32_t cluster = dir_cluster;
    
    do {
        // Read the cluster
        if (fat32_read_cluster(fs, cluster, cluster_data) != 0) {
            return -1;
        }
        
        // Iterate through directory entries
        fat32_dir_entry_t* entries = (fat32_dir_entry_t*)cluster_data;
        for (uint32_t i = 0; i < fs->bytes_per_cluster / sizeof(fat32_dir_entry_t); i++) {
            // Check for end of directory
            if (entries[i].name[0] == 0x00) {
                return -1;  // Entry not found
            }
            
            // Skip deleted entries
            if (entries[i].name[0] == 0xE5) {
                continue;
            }
            
            // Skip long filename entries
            if ((entries[i].attr & FAT32_ATTR_LONG_NAME) == FAT32_ATTR_LONG_NAME) {
                continue;
            }
            
            // Compare names
            if (fat32_compare_name(entries[i].name, name)) {
                if (entry) {
                    memcpy(entry, &entries[i], sizeof(fat32_dir_entry_t));
                }
                return 0;  // Found
            }
        }
        
        // Get next cluster
        cluster = fat32_get_next_cluster(fs, cluster);
        
    } while (cluster < FAT32_END_OF_CHAIN);
    
    return -1;  // Not found
}

// Open a file or directory
int fat32_open(fat32_file_t* file, const char* path) {
    if (!file || !path) return -1;
    
    // Parse the path
    char components[32][FAT32_MAX_FILENAME];
    int num_components = fat32_parse_path(path, components, 32);
    
    if (num_components == 0) {
        // Root directory
        file->first_cluster = file->fs->root_cluster;
        file->current_cluster = file->fs->root_cluster;
        file->current_offset = 0;
        file->file_size = 0;
        file->is_directory = 1;
        file->name[0] = '/';
        file->name[1] = '\0';
        return 0;
    }
    
    // Traverse the path
    uint32_t current_cluster = file->fs->root_cluster;
    fat32_dir_entry_t entry;
    
    for (int i = 0; i < num_components; i++) {
        if (fat32_find_entry(file->fs, current_cluster, components[i], &entry) != 0) {
            return -1;  // Not found
        }
        
        // Get the cluster for this entry
        uint32_t entry_cluster = ((uint32_t)entry.cluster_high << 16) | entry.cluster_low;
        
        if (i == num_components - 1) {
            // This is the target file/directory
            file->first_cluster = entry_cluster;
            file->current_cluster = entry_cluster;
            file->current_offset = 0;
            file->file_size = entry.file_size;
            file->is_directory = (entry.attr & FAT32_ATTR_DIRECTORY) ? 1 : 0;
            strncpy(file->name, components[i], FAT32_MAX_FILENAME - 1);
            file->name[FAT32_MAX_FILENAME - 1] = '\0';
            return 0;
        }
        
        // Must be a directory to continue
        if (!(entry.attr & FAT32_ATTR_DIRECTORY)) {
            return -1;
        }
        
        current_cluster = entry_cluster;
    }
    
    return -1;
}

// Read from a file
int fat32_read(fat32_file_t* file, void* buffer, uint32_t size) {
    if (!file || !buffer || file->is_directory) return -1;
    
    uint8_t* buf = (uint8_t*)buffer;
    uint32_t bytes_read = 0;
    uint8_t cluster_data[4096];
    
    while (bytes_read < size && file->current_offset < file->file_size) {
        // Calculate offset within cluster
        uint32_t cluster_offset = file->current_offset % file->fs->bytes_per_cluster;
        
        // Read current cluster if needed
        if (cluster_offset == 0 || file->current_cluster != file->first_cluster) {
            if (fat32_read_cluster(file->fs, file->current_cluster, cluster_data) != 0) {
                break;
            }
        }
        
        // Calculate how many bytes to copy
        uint32_t bytes_left_in_cluster = file->fs->bytes_per_cluster - cluster_offset;
        uint32_t bytes_left_in_file = file->file_size - file->current_offset;
        uint32_t bytes_to_copy = size - bytes_read;
        if (bytes_to_copy > bytes_left_in_cluster) bytes_to_copy = bytes_left_in_cluster;
        if (bytes_to_copy > bytes_left_in_file) bytes_to_copy = bytes_left_in_file;
        
        // Copy data
        memcpy(buf + bytes_read, cluster_data + cluster_offset, bytes_to_copy);
        
        bytes_read += bytes_to_copy;
        file->current_offset += bytes_to_copy;
        
        // Move to next cluster if needed
        if (file->current_offset % file->fs->bytes_per_cluster == 0) {
            file->current_cluster = fat32_get_next_cluster(file->fs, file->current_cluster);
            if (file->current_cluster >= FAT32_END_OF_CHAIN) {
                break;
            }
        }
    }
    
    return bytes_read;
}

// Write to a file
int fat32_write(fat32_file_t* file, const void* buffer, uint32_t size) {
    if (!file || !buffer || file->is_directory) return -1;
    
    const uint8_t* buf = (const uint8_t*)buffer;
    uint32_t bytes_written = 0;
    uint8_t cluster_data[4096];
    
    while (bytes_written < size) {
        // Calculate offset within cluster
        uint32_t cluster_offset = file->current_offset % file->fs->bytes_per_cluster;
        
        // Read current cluster for partial writes
        if (cluster_offset != 0) {
            if (fat32_read_cluster(file->fs, file->current_cluster, cluster_data) != 0) {
                break;
            }
        }
        
        // Calculate how many bytes to copy
        uint32_t bytes_left_in_cluster = file->fs->bytes_per_cluster - cluster_offset;
        uint32_t bytes_to_copy = size - bytes_written;
        if (bytes_to_copy > bytes_left_in_cluster) bytes_to_copy = bytes_left_in_cluster;
        
        // Copy data to cluster buffer
        memcpy(cluster_data + cluster_offset, buf + bytes_written, bytes_to_copy);
        
        // Write cluster back
        if (fat32_write_cluster(file->fs, file->current_cluster, cluster_data) != 0) {
            break;
        }
        
        bytes_written += bytes_to_copy;
        file->current_offset += bytes_to_copy;
        
        // Extend file if needed
        if (file->current_offset > file->file_size) {
            file->file_size = file->current_offset;
        }
        
        // Move to next cluster if needed
        if (file->current_offset % file->fs->bytes_per_cluster == 0 && 
            bytes_written < size) {
            uint32_t next_cluster = fat32_get_next_cluster(file->fs, file->current_cluster);
            
            if (next_cluster >= FAT32_END_OF_CHAIN) {
                // Need to allocate a new cluster
                next_cluster = fat32_find_free_cluster(file->fs);
                if (next_cluster == 0) {
                    break;  // Disk full
                }
                fat32_set_fat_entry(file->fs, file->current_cluster, next_cluster);
                fat32_set_fat_entry(file->fs, next_cluster, FAT32_END_OF_CHAIN);
            }
            
            file->current_cluster = next_cluster;
        }
    }
    
    return bytes_written;
}

// Close a file
void fat32_close(fat32_file_t* file) {
    // Nothing to do for now
    (void)file;
}

// List directory contents
int fat32_list_directory(fat32_fs_t* fs, const char* path) {
    if (!fs || !fs->mounted) return -1;
    
    uint32_t cluster = fs->root_cluster;
    
    // Navigate to the directory
    if (path && path[0] && strcmp(path, "/") != 0) {
        fat32_file_t dir;
        dir.fs = fs;
        if (fat32_open(&dir, path) != 0) {
            terminal_writestring("FAT32: Directory not found\n");
            return -1;
        }
        if (!dir.is_directory) {
            terminal_writestring("FAT32: Not a directory\n");
            return -1;
        }
        cluster = dir.first_cluster;
    }
    
    uint8_t cluster_data[4096];
    char name[13];
    
    do {
        // Read the cluster
        if (fat32_read_cluster(fs, cluster, cluster_data) != 0) {
            return -1;
        }
        
        // Iterate through directory entries
        fat32_dir_entry_t* entries = (fat32_dir_entry_t*)cluster_data;
        for (uint32_t i = 0; i < fs->bytes_per_cluster / sizeof(fat32_dir_entry_t); i++) {
            // Check for end of directory
            if (entries[i].name[0] == 0x00) {
                return 0;
            }
            
            // Skip deleted entries
            if (entries[i].name[0] == 0xE5) {
                continue;
            }
            
            // Skip long filename entries
            if ((entries[i].attr & FAT32_ATTR_LONG_NAME) == FAT32_ATTR_LONG_NAME) {
                continue;
            }
            
            // Skip volume label
            if (entries[i].attr & FAT32_ATTR_VOLUME_ID) {
                continue;
            }
            
            // Print the entry
            fat32_name_to_string(entries[i].name, name);
            terminal_writestring(name);
            
            if (entries[i].attr & FAT32_ATTR_DIRECTORY) {
                terminal_writestring("/");
            }
            
            terminal_writestring("  ");
        }
        
        // Get next cluster
        cluster = fat32_get_next_cluster(fs, cluster);
        
    } while (cluster < FAT32_END_OF_CHAIN);
    
    terminal_writestring("\n");
    return 0;
}

// Check if path exists
int fat32_exists(fat32_fs_t* fs, const char* path) {
    if (!fs || !fs->mounted) return 0;
    
    fat32_file_t file;
    file.fs = fs;
    return (fat32_open(&file, path) == 0) ? 1 : 0;
}

// Create a file
int fat32_create(fat32_fs_t* fs, const char* path) {
    if (!fs || !fs->mounted || !path) return -1;
    
    // Parse the path
    char components[32][FAT32_MAX_FILENAME];
    int num_components = fat32_parse_path(path, components, 32);
    
    if (num_components == 0) return -1;
    
    // Find the parent directory
    uint32_t parent_cluster = fs->root_cluster;
    for (int i = 0; i < num_components - 1; i++) {
        fat32_dir_entry_t entry;
        if (fat32_find_entry(fs, parent_cluster, components[i], &entry) != 0) {
            return -1;  // Parent not found
        }
        if (!(entry.attr & FAT32_ATTR_DIRECTORY)) {
            return -1;  // Not a directory
        }
        parent_cluster = ((uint32_t)entry.cluster_high << 16) | entry.cluster_low;
    }
    
    // Check if file already exists
    fat32_dir_entry_t existing;
    if (fat32_find_entry(fs, parent_cluster, components[num_components - 1], &existing) == 0) {
        terminal_writestring("FAT32: File already exists\n");
        return -1;
    }
    
    // Find a free cluster for the new file
    uint32_t new_cluster = fat32_find_free_cluster(fs);
    if (new_cluster == 0) {
        terminal_writestring("FAT32: Disk full\n");
        return -1;
    }
    
    // Mark the cluster as end of chain
    fat32_set_fat_entry(fs, new_cluster, FAT32_END_OF_CHAIN);
    
    // Create the directory entry
    uint8_t cluster_data[4096];
    if (fat32_read_cluster(fs, parent_cluster, cluster_data) != 0) {
        return -1;
    }
    
    fat32_dir_entry_t* entries = (fat32_dir_entry_t*)cluster_data;
    int found_slot = 0;
    
    for (uint32_t i = 0; i < fs->bytes_per_cluster / sizeof(fat32_dir_entry_t); i++) {
        if (entries[i].name[0] == 0x00 || entries[i].name[0] == 0xE5) {
            // Found a free slot
            memset(&entries[i], 0, sizeof(fat32_dir_entry_t));
            string_to_fat_name(components[num_components - 1], entries[i].name);
            entries[i].attr = FAT32_ATTR_ARCHIVE;
            entries[i].cluster_high = (new_cluster >> 16) & 0xFFFF;
            entries[i].cluster_low = new_cluster & 0xFFFF;
            entries[i].file_size = 0;
            found_slot = 1;
            break;
        }
    }
    
    if (!found_slot) {
        terminal_writestring("FAT32: Directory full\n");
        return -1;
    }
    
    // Write the directory cluster back
    if (fat32_write_cluster(fs, parent_cluster, cluster_data) != 0) {
        return -1;
    }
    
    return 0;
}

// Create a directory
int fat32_mkdir(fat32_fs_t* fs, const char* path) {
    if (!fs || !fs->mounted || !path) return -1;
    
    // Parse the path
    char components[32][FAT32_MAX_FILENAME];
    int num_components = fat32_parse_path(path, components, 32);
    
    if (num_components == 0) return -1;
    
    // Find the parent directory
    uint32_t parent_cluster = fs->root_cluster;
    for (int i = 0; i < num_components - 1; i++) {
        fat32_dir_entry_t entry;
        if (fat32_find_entry(fs, parent_cluster, components[i], &entry) != 0) {
            return -1;  // Parent not found
        }
        if (!(entry.attr & FAT32_ATTR_DIRECTORY)) {
            return -1;  // Not a directory
        }
        parent_cluster = ((uint32_t)entry.cluster_high << 16) | entry.cluster_low;
    }
    
    // Check if directory already exists
    fat32_dir_entry_t existing;
    if (fat32_find_entry(fs, parent_cluster, components[num_components - 1], &existing) == 0) {
        terminal_writestring("FAT32: Directory already exists\n");
        return -1;
    }
    
    // Find a free cluster for the new directory
    uint32_t new_cluster = fat32_find_free_cluster(fs);
    if (new_cluster == 0) {
        terminal_writestring("FAT32: Disk full\n");
        return -1;
    }
    
    // Mark the cluster as end of chain
    fat32_set_fat_entry(fs, new_cluster, FAT32_END_OF_CHAIN);
    
    // Initialize the directory with . and .. entries
    uint8_t cluster_data[4096];
    memset(cluster_data, 0, fs->bytes_per_cluster);
    
    fat32_dir_entry_t* entries = (fat32_dir_entry_t*)cluster_data;
    
    // "." entry
    memset(entries[0].name, ' ', 11);
    entries[0].name[0] = '.';
    entries[0].attr = FAT32_ATTR_DIRECTORY;
    entries[0].cluster_high = (new_cluster >> 16) & 0xFFFF;
    entries[0].cluster_low = new_cluster & 0xFFFF;
    entries[0].file_size = 0;
    
    // ".." entry
    memset(entries[1].name, ' ', 11);
    entries[1].name[0] = '.';
    entries[1].name[1] = '.';
    entries[1].attr = FAT32_ATTR_DIRECTORY;
    entries[1].cluster_high = (parent_cluster >> 16) & 0xFFFF;
    entries[1].cluster_low = parent_cluster & 0xFFFF;
    entries[1].file_size = 0;
    
    // Write the new directory cluster
    if (fat32_write_cluster(fs, new_cluster, cluster_data) != 0) {
        return -1;
    }
    
    // Add entry in parent directory
    if (fat32_read_cluster(fs, parent_cluster, cluster_data) != 0) {
        return -1;
    }
    
    int found_slot = 0;
    for (uint32_t i = 0; i < fs->bytes_per_cluster / sizeof(fat32_dir_entry_t); i++) {
        if (entries[i].name[0] == 0x00 || entries[i].name[0] == 0xE5) {
            memset(&entries[i], 0, sizeof(fat32_dir_entry_t));
            string_to_fat_name(components[num_components - 1], entries[i].name);
            entries[i].attr = FAT32_ATTR_DIRECTORY;
            entries[i].cluster_high = (new_cluster >> 16) & 0xFFFF;
            entries[i].cluster_low = new_cluster & 0xFFFF;
            entries[i].file_size = 0;
            found_slot = 1;
            break;
        }
    }
    
    if (!found_slot) {
        terminal_writestring("FAT32: Directory full\n");
        return -1;
    }
    
    // Write the parent directory cluster back
    if (fat32_write_cluster(fs, parent_cluster, cluster_data) != 0) {
        return -1;
    }
    
    return 0;
}

// Delete a file
int fat32_delete(fat32_fs_t* fs, const char* path) {
    if (!fs || !fs->mounted || !path) return -1;
    
    // Parse the path
    char components[32][FAT32_MAX_FILENAME];
    int num_components = fat32_parse_path(path, components, 32);
    
    if (num_components == 0) return -1;
    
    // Find the parent directory
    uint32_t parent_cluster = fs->root_cluster;
    for (int i = 0; i < num_components - 1; i++) {
        fat32_dir_entry_t entry;
        if (fat32_find_entry(fs, parent_cluster, components[i], &entry) != 0) {
            return -1;
        }
        if (!(entry.attr & FAT32_ATTR_DIRECTORY)) {
            return -1;
        }
        parent_cluster = ((uint32_t)entry.cluster_high << 16) | entry.cluster_low;
    }
    
    // Find the file entry
    uint8_t cluster_data[4096];
    uint32_t cluster = parent_cluster;
    int found = 0;
    uint32_t file_cluster = 0;
    
    do {
        if (fat32_read_cluster(fs, cluster, cluster_data) != 0) {
            return -1;
        }
        
        fat32_dir_entry_t* entries = (fat32_dir_entry_t*)cluster_data;
        for (uint32_t i = 0; i < fs->bytes_per_cluster / sizeof(fat32_dir_entry_t); i++) {
            if (entries[i].name[0] == 0x00) {
                return -1;  // File not found
            }
            
            if (entries[i].name[0] == 0xE5) continue;
            if ((entries[i].attr & FAT32_ATTR_LONG_NAME) == FAT32_ATTR_LONG_NAME) continue;
            
            if (fat32_compare_name(entries[i].name, components[num_components - 1])) {
                if (entries[i].attr & FAT32_ATTR_DIRECTORY) {
                    terminal_writestring("FAT32: Cannot delete directory\n");
                    return -1;
                }
                
                file_cluster = ((uint32_t)entries[i].cluster_high << 16) | entries[i].cluster_low;
                entries[i].name[0] = 0xE5;  // Mark as deleted
                found = 1;
                break;
            }
        }
        
        if (found) {
            // Write the modified cluster back
            if (fat32_write_cluster(fs, cluster, cluster_data) != 0) {
                return -1;
            }
            break;
        }
        
        cluster = fat32_get_next_cluster(fs, cluster);
    } while (cluster < FAT32_END_OF_CHAIN);
    
    if (!found) {
        terminal_writestring("FAT32: File not found\n");
        return -1;
    }
    
    // Free all clusters in the chain
    while (file_cluster && file_cluster < FAT32_END_OF_CHAIN) {
        uint32_t next = fat32_get_next_cluster(fs, file_cluster);
        fat32_set_fat_entry(fs, file_cluster, FAT32_FREE_CLUSTER);
        file_cluster = next;
    }
    
    return 0;
}

// Get file size
int fat32_get_file_size(fat32_fs_t* fs, const char* path) {
    if (!fs || !fs->mounted || !path) return -1;
    
    fat32_file_t file;
    file.fs = fs;
    if (fat32_open(&file, path) != 0) {
        return -1;
    }
    
    return (int)file.file_size;
}

// Read entire cluster chain
uint32_t fat32_read_cluster_chain(fat32_fs_t* fs, uint32_t start_cluster, 
                                   uint8_t* buffer, uint32_t max_bytes) {
    if (!fs || !buffer || start_cluster < 2) return 0;
    
    uint32_t bytes_read = 0;
    uint32_t cluster = start_cluster;
    
    while (cluster < FAT32_END_OF_CHAIN && bytes_read < max_bytes) {
        uint32_t to_read = fs->bytes_per_cluster;
        if (bytes_read + to_read > max_bytes) {
            to_read = max_bytes - bytes_read;
        }
        
        if (fat32_read_cluster(fs, cluster, buffer + bytes_read) != 0) {
            break;
        }
        
        bytes_read += to_read;
        cluster = fat32_get_next_cluster(fs, cluster);
    }
    
    return bytes_read;
}