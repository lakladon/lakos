#include <stdint.h>
#include <stddef.h>
#include "include/lib.h"
#include "include/fat32.h"

extern void terminal_writestring(const char* s);

// Helper function to convert a cluster number to a sector offset
static uint32_t fat32_cluster_to_sector(fat32_context_t* ctx, uint32_t cluster) {
    return ctx->data_start_sector + (cluster - 2) * ctx->sectors_per_cluster;
}

// Helper function to read FAT entry for a given cluster
static uint32_t fat32_read_fat_entry(fat32_context_t* ctx, uint32_t cluster) {
    uint32_t fat_offset = cluster * 4; // Each FAT32 entry is 4 bytes
    uint32_t fat_sector = ctx->fat_start_sector + (fat_offset / ctx->bytes_per_sector);
    uint32_t sector_offset = fat_offset % ctx->bytes_per_sector;
    
    uint8_t* fat_data = (uint8_t*)ctx->disk_buffer + (fat_sector * ctx->bytes_per_sector);
    uint32_t next_cluster = *(uint32_t*)(fat_data + sector_offset);
    
    return next_cluster & 0x0FFFFFFF; // Mask off top 4 bits
}

// Helper function to get file size in a cluster chain
static int fat32_get_cluster_chain_size(fat32_context_t* ctx, uint32_t start_cluster) {
    if (start_cluster == 0) {
        return 0;
    }
    
    int size = 0;
    uint32_t current_cluster = start_cluster;
    
    // Count clusters until we reach the end of the chain
    while (current_cluster < 0x0FFFFFF8) {
        size += ctx->cluster_size;
        current_cluster = fat32_read_fat_entry(ctx, current_cluster);
    }
    
    return size;
}

// Initialize FAT32 context from disk image
void* fat32_init(void* disk_image) {
    if (!disk_image) {
        return NULL;
    }
    
    fat32_boot_sector_t* boot = (fat32_boot_sector_t*)disk_image;
    
    // Verify it looks like a FAT32 filesystem
    if (boot->bytes_per_sector == 0 || boot->sectors_per_cluster == 0) {
        return NULL;
    }
    
    // Allocate a static context (simplified approach for kernel)
    static fat32_context_t ctx;
    
    // Initialize context
    ctx.disk_buffer = disk_image;
    ctx.bytes_per_sector = boot->bytes_per_sector;
    ctx.sectors_per_cluster = boot->sectors_per_cluster;
    ctx.cluster_size = boot->bytes_per_sector * boot->sectors_per_cluster;
    ctx.fat_start_sector = boot->reserved_sectors;
    ctx.data_start_sector = boot->reserved_sectors + 
                             (boot->num_fats * boot->sectors_per_fat);
    ctx.root_cluster = boot->root_cluster;
    
    return (void*)&ctx;
}

// Parse a path into components
static int fat32_parse_path(const char* path, char components[][256], int max_components) {
    if (!path || path[0] == '\0') {
        return 0;
    }
    
    int count = 0;
    const char* start = path;
    
    // Skip leading slash
    if (start[0] == '/') {
        start++;
    }
    
    while (*start != '\0' && count < max_components) {
        const char* end = start;
        while (*end != '\0' && *end != '/') {
            end++;
        }
        
        int len = end - start;
        if (len > 0 && len < 256) {
            strncpy(components[count], start, len);
            components[count][len] = '\0';
            count++;
        }
        
        start = (*end == '/') ? end + 1 : end;
    }
    
    return count;
}

// Get directory entry by name in a given cluster
static fat32_dir_entry_t* fat32_find_entry_in_cluster(fat32_context_t* ctx, 
                                                       uint32_t cluster, 
                                                       const char* name) {
    uint32_t sector = fat32_cluster_to_sector(ctx, cluster);
    uint8_t* cluster_data = (uint8_t*)ctx->disk_buffer + (sector * ctx->bytes_per_sector);
    
    fat32_dir_entry_t* entries = (fat32_dir_entry_t*)cluster_data;
    int entries_per_cluster = ctx->cluster_size / sizeof(fat32_dir_entry_t);
    
    for (int i = 0; i < entries_per_cluster; i++) {
        // Check if entry is empty
        if (entries[i].filename[0] == 0x00) {
            break;
        }
        
        // Check if entry is deleted
        if ((uint8_t)entries[i].filename[0] == 0xE5) {
            continue;
        }
        
        // Skip LFN entries
        if (entries[i].attributes == 0x0F) {
            continue;
        }
        
        // Build 8.3 name
        char short_name[13];
        int pos = 0;
        
        // Copy filename, stripping trailing spaces
        for (int j = 0; j < 8 && entries[i].filename[j] != ' '; j++) {
            short_name[pos++] = entries[i].filename[j];
        }
        
        // Add extension if present
        if (entries[i].extension[0] != ' ') {
            short_name[pos++] = '.';
            for (int j = 0; j < 3 && entries[i].extension[j] != ' '; j++) {
                short_name[pos++] = entries[i].extension[j];
            }
        }
        short_name[pos] = '\0';
        
        // Compare names (case-insensitive)
        if (strcmp(short_name, name) == 0) {
            return &entries[i];
        }
    }
    
    return NULL;
}

// Check if path exists in FAT32 filesystem
int fat32_check_path_exists(void* context, const char* path) {
    if (!context || !path) {
        return 0;
    }
    
    fat32_context_t* ctx = (fat32_context_t*)context;
    
    // Root always exists
    if (path[0] == '/' && path[1] == '\0') {
        return 1;
    }
    
    char components[16][256];
    int comp_count = fat32_parse_path(path, components, 16);
    
    uint32_t current_cluster = ctx->root_cluster;
    
    for (int i = 0; i < comp_count; i++) {
        fat32_dir_entry_t* entry = fat32_find_entry_in_cluster(ctx, current_cluster, components[i]);
        if (!entry) {
            return 0; // Not found
        }
        
        if (i < comp_count - 1) {
            // Not the last component, must be a directory
            if (!(entry->attributes & 0x10)) {
                return 0; // Not a directory
            }
            current_cluster = ((uint32_t)entry->high_cluster << 16) | entry->low_cluster;
        }
    }
    
    return 1;
}

// Lookup a file and return pointer to its data
void* fat32_lookup(void* context, const char* filename) {
    if (!context || !filename) {
        return NULL;
    }
    
    fat32_context_t* ctx = (fat32_context_t*)context;
    
    char components[16][256];
    int comp_count = fat32_parse_path(filename, components, 16);
    
    if (comp_count == 0) {
        return NULL;
    }
    
    uint32_t current_cluster = ctx->root_cluster;
    
    for (int i = 0; i < comp_count; i++) {
        fat32_dir_entry_t* entry = fat32_find_entry_in_cluster(ctx, current_cluster, components[i]);
        if (!entry) {
            return NULL;
        }
        
        if (i == comp_count - 1) {
            // Found the file
            uint32_t start_cluster = ((uint32_t)entry->high_cluster << 16) | entry->low_cluster;
            if (start_cluster == 0) {
                return NULL;
            }
            
            uint32_t sector = fat32_cluster_to_sector(ctx, start_cluster);
            return (void*)((uint8_t*)ctx->disk_buffer + (sector * ctx->bytes_per_sector));
        }
        
        // Not the last component, must be a directory
        if (!(entry->attributes & 0x10)) {
            return NULL;
        }
        current_cluster = ((uint32_t)entry->high_cluster << 16) | entry->low_cluster;
    }
    
    return NULL;
}

// Get file size
int fat32_get_file_size(void* context, const char* filename) {
    if (!context || !filename) {
        return -1;
    }
    
    fat32_context_t* ctx = (fat32_context_t*)context;
    
    char components[16][256];
    int comp_count = fat32_parse_path(filename, components, 16);
    
    if (comp_count == 0) {
        return -1;
    }
    
    uint32_t current_cluster = ctx->root_cluster;
    
    for (int i = 0; i < comp_count; i++) {
        fat32_dir_entry_t* entry = fat32_find_entry_in_cluster(ctx, current_cluster, components[i]);
        if (!entry) {
            return -1;
        }
        
        if (i == comp_count - 1) {
            // Found the file, return its size
            return (int)entry->file_size;
        }
        
        if (!(entry->attributes & 0x10)) {
            return -1;
        }
        current_cluster = ((uint32_t)entry->high_cluster << 16) | entry->low_cluster;
    }
    
    return -1;
}

// List all files in root directory
void fat32_list_files(void* context) {
    if (!context) {
        return;
    }
    
    fat32_context_t* ctx = (fat32_context_t*)context;
    fat32_list_directory(context, "/");
}

// List files in a directory
void fat32_list_directory(void* context, const char* dirpath) {
    if (!context) {
        return;
    }
    
    fat32_context_t* ctx = (fat32_context_t*)context;
    
    uint32_t cluster = ctx->root_cluster;
    
    // If not listing root, find the directory
    if (dirpath && dirpath[0] != '\0' && !(dirpath[0] == '/' && dirpath[1] == '\0')) {
        char components[16][256];
        int comp_count = fat32_parse_path(dirpath, components, 16);
        
        for (int i = 0; i < comp_count; i++) {
            fat32_dir_entry_t* entry = fat32_find_entry_in_cluster(ctx, cluster, components[i]);
            if (!entry || !(entry->attributes & 0x10)) {
                return; // Not found or not a directory
            }
            cluster = ((uint32_t)entry->high_cluster << 16) | entry->low_cluster;
        }
    }
    
    // List entries in the cluster
    uint32_t sector = fat32_cluster_to_sector(ctx, cluster);
    uint8_t* cluster_data = (uint8_t*)ctx->disk_buffer + (sector * ctx->bytes_per_sector);
    
    fat32_dir_entry_t* entries = (fat32_dir_entry_t*)cluster_data;
    int entries_per_cluster = ctx->cluster_size / sizeof(fat32_dir_entry_t);
    
    for (int i = 0; i < entries_per_cluster; i++) {
        if (entries[i].filename[0] == 0x00) {
            break;
        }
        
        if ((uint8_t)entries[i].filename[0] == 0xE5 || entries[i].attributes == 0x0F) {
            continue;
        }
        
        // Build name
        char name[13];
        int pos = 0;
        
        for (int j = 0; j < 8 && entries[i].filename[j] != ' '; j++) {
            name[pos++] = entries[i].filename[j];
        }
        
        if (entries[i].extension[0] != ' ') {
            name[pos++] = '.';
            for (int j = 0; j < 3 && entries[i].extension[j] != ' '; j++) {
                name[pos++] = entries[i].extension[j];
            }
        }
        name[pos] = '\0';
        
        terminal_writestring(name);
        if (entries[i].attributes & 0x10) {
            terminal_writestring("/ ");
        } else {
            terminal_writestring(" ");
        }
    }
    terminal_writestring("\n");
}

// Get all directories
void fat32_get_directories(void* context, char directories[][256], int* count) {
    if (!context || !count) {
        return;
    }
    
    fat32_context_t* ctx = (fat32_context_t*)context;
    *count = 0;
    
    uint32_t sector = fat32_cluster_to_sector(ctx, ctx->root_cluster);
    uint8_t* cluster_data = (uint8_t*)ctx->disk_buffer + (sector * ctx->bytes_per_sector);
    
    fat32_dir_entry_t* entries = (fat32_dir_entry_t*)cluster_data;
    int entries_per_cluster = ctx->cluster_size / sizeof(fat32_dir_entry_t);
    
    for (int i = 0; i < entries_per_cluster && *count < 100; i++) {
        if (entries[i].filename[0] == 0x00) {
            break;
        }
        
        if ((uint8_t)entries[i].filename[0] == 0xE5 || entries[i].attributes == 0x0F) {
            continue;
        }
        
        // Only add directories
        if (!(entries[i].attributes & 0x10)) {
            continue;
        }
        
        // Skip . and ..
        if ((entries[i].filename[0] == '.' && entries[i].filename[1] == ' ') ||
            (entries[i].filename[0] == '.' && entries[i].filename[1] == '.' && entries[i].filename[2] == ' ')) {
            continue;
        }
        
        // Build directory name
        int pos = 0;
        for (int j = 0; j < 8 && entries[i].filename[j] != ' '; j++) {
            directories[*count][pos++] = entries[i].filename[j];
        }
        directories[*count][pos] = '\0';
        (*count)++;
    }
}
