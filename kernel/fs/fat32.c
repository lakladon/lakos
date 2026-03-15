extern void terminal_writestring(const char* s)
extern void ata_read_sector(uint8_t drive, uint32_t lba, uint16_t* buffer)
extern void ata_write_sector(uint8_t drive, uint32_t lba, uint16_t* buffer)
static fat32_fs_t mounted_fs[MAX_FAT32_MOUNTS]
static uint16_t sector_buffer[256]
static uint8_t cluster_buffer[4096]
void fat32_init(void) {
    for (int i = 0
        mounted_fs[i].mounted = 0
        mounted_fs[i].mount_point[0] = '\0'
    }
}
fat32_fs_t* fat32_get_mounted_fs(const char* mount_point) {
    for (int i = 0
        if (mounted_fs[i].mounted && 
            strcmp(mounted_fs[i].mount_point, mount_point) == 0) {
            return &mounted_fs[i]
        }
    }
    return NULL
}
fat32_fs_t* fat32_get_mounted_fs_by_index(int index) {
    if (index < 0 || index >= MAX_FAT32_MOUNTS) {
        return NULL
    }
    return &mounted_fs[index]
}
static int fat32_read_sector(fat32_fs_t* fs, uint32_t sector, uint8_t* buffer) {
    uint32_t lba = fs->partition_start + sector
    ata_read_sector(fs->drive, lba, (uint16_t*)sector_buffer)
    memcpy(buffer, sector_buffer, 512)
    return 0
}
static int fat32_write_sector(fat32_fs_t* fs, uint32_t sector, const uint8_t* buffer) {
    uint32_t lba = fs->partition_start + sector
    memcpy(sector_buffer, buffer, 512)
    ata_write_sector(fs->drive, lba, sector_buffer)
    return 0
}
static int fat32_read_cluster(fat32_fs_t* fs, uint32_t cluster, uint8_t* buffer) {
    uint32_t first_sector = fs->data_start + 
                            (cluster - 2) * fs->sectors_per_cluster
    for (uint32_t i = 0
        if (fat32_read_sector(fs, first_sector + i, 
                              buffer + i * fs->bytes_per_sector) != 0) {
            return -1
        }
    }
    return 0
}
static int fat32_write_cluster(fat32_fs_t* fs, uint32_t cluster, const uint8_t* buffer) {
    uint32_t first_sector = fs->data_start + 
                            (cluster - 2) * fs->sectors_per_cluster
    for (uint32_t i = 0
        if (fat32_write_sector(fs, first_sector + i, 
                               buffer + i * fs->bytes_per_sector) != 0) {
            return -1
        }
    }
    return 0
}
uint32_t fat32_get_next_cluster(fat32_fs_t* fs, uint32_t cluster) {
    if (cluster < 2 || cluster >= fs->total_clusters + 2) {
        return FAT32_BAD_CLUSTER
    }
    uint32_t fat_offset = cluster * 4
    uint32_t fat_sector = fs->fat_start + fat_offset / fs->bytes_per_sector
    uint32_t fat_offset_in_sector = fat_offset % fs->bytes_per_sector
    uint8_t fat_sector_data[512]
    if (fat32_read_sector(fs, fat_sector, fat_sector_data) != 0) {
        return FAT32_BAD_CLUSTER
    }
    uint32_t fat_entry = *((uint32_t*)(fat_sector_data + fat_offset_in_sector))
    fat_entry &= 0x0FFFFFFF
    return fat_entry
}
static int fat32_set_fat_entry(fat32_fs_t* fs, uint32_t cluster, uint32_t value) {
    if (cluster < 2 || cluster >= fs->total_clusters + 2) {
        return -1
    }
    value &= 0x0FFFFFFF
    uint32_t fat_offset = cluster * 4
    uint32_t fat_sector = fs->fat_start + fat_offset / fs->bytes_per_sector
    uint32_t fat_offset_in_sector = fat_offset % fs->bytes_per_sector
    uint8_t fat_sector_data[512]
    if (fat32_read_sector(fs, fat_sector, fat_sector_data) != 0) {
        return -1
    }
    uint32_t* entry = (uint32_t*)(fat_sector_data + fat_offset_in_sector)
    *entry = (*entry & 0xF0000000) | value
    if (fat32_write_sector(fs, fat_sector, fat_sector_data) != 0) {
        return -1
    }
    return 0
}
static uint32_t fat32_find_free_cluster(fat32_fs_t* fs) {
    for (uint32_t cluster = 2
        if (fat32_get_next_cluster(fs, cluster) == FAT32_FREE_CLUSTER) {
            return cluster
        }
    }
    return 0
}
int fat32_mount(uint8_t drive, uint32_t partition_start, const char* mount_point) {
    int slot = -1
    for (int i = 0
        if (!mounted_fs[i].mounted) {
            slot = i
            break
        }
    }
    if (slot < 0) {
        terminal_writestring("FAT32: No free mount slots\n")
        return -1
    }
    fat32_boot_sector_t boot_sector
    ata_read_sector(drive, partition_start, (uint16_t*)sector_buffer)
    memcpy(&boot_sector, sector_buffer, sizeof(fat32_boot_sector_t))
    if (boot_sector.boot_signature != 0x29 && boot_sector.boot_signature != 0x28) {
        terminal_writestring("FAT32: Invalid boot signature\n")
        return -1
    }
    if (boot_sector.fat_size_16 != 0) {
        terminal_writestring("FAT32: Not a FAT32 filesystem (FAT12/16 detected)\n")
        return -1
    }
    if (strncmp((char*)boot_sector.fs_type, "FAT32", 5) != 0) {
        terminal_writestring("FAT32: FS type mismatch\n")
    }
    fat32_fs_t* fs = &mounted_fs[slot]
    fs->drive = drive
    fs->partition_start = partition_start
    fs->bytes_per_sector = boot_sector.bytes_per_sector
    fs->sectors_per_cluster = boot_sector.sectors_per_cluster
    fs->bytes_per_cluster = fs->bytes_per_sector * fs->sectors_per_cluster
    fs->reserved_sector_count = boot_sector.reserved_sector_count
    fs->fat_size = boot_sector.fat_size_32
    fs->num_fats = boot_sector.num_fats
    fs->root_cluster = boot_sector.root_cluster
    fs->fat_start = fs->reserved_sector_count
    fs->data_start = fs->reserved_sector_count + 
                     (fs->num_fats * fs->fat_size)
    uint32_t total_sectors = boot_sector.total_sectors_16 ? 
                             boot_sector.total_sectors_16 : 
                             boot_sector.total_sectors_32
    uint32_t data_sectors = total_sectors - fs->data_start
    fs->total_clusters = data_sectors / fs->sectors_per_cluster
    strncpy(fs->mount_point, mount_point, 63)
    fs->mount_point[63] = '\0'
    fs->mounted = 1
    terminal_writestring("FAT32: Mounted ")
    terminal_writestring(mount_point)
    terminal_writestring(" on drive ")
    char buf[16]
    itoa(drive, buf)
    terminal_writestring(buf)
    terminal_writestring(", partition ")
    itoa(partition_start, buf)
    terminal_writestring(buf)
    terminal_writestring("\n  Bytes per sector: ")
    itoa(fs->bytes_per_sector, buf)
    terminal_writestring(buf)
    terminal_writestring("\n  Sectors per cluster: ")
    itoa(fs->sectors_per_cluster, buf)
    terminal_writestring(buf)
    terminal_writestring("\n  Total clusters: ")
    itoa(fs->total_clusters, buf)
    terminal_writestring(buf)
    terminal_writestring("\n  Volume label: ")
    char label[12]
    strncpy(label, (char*)boot_sector.volume_label, 11)
    label[11] = '\0'
    terminal_writestring(label)
    terminal_writestring("\n")
    return 0
}
int fat32_unmount(const char* mount_point) {
    fat32_fs_t* fs = fat32_get_mounted_fs(mount_point)
    if (!fs) {
        terminal_writestring("FAT32: Mount point not found\n")
        return -1
    }
    fs->mounted = 0
    fs->mount_point[0] = '\0'
    terminal_writestring("FAT32: Unmounted ")
    terminal_writestring(mount_point)
    terminal_writestring("\n")
    return 0
}
static void fat32_name_to_string(const uint8_t* fat_name, char* out) {
    int i = 0
    int j = 0
    for (i = 0
        out[j++] = fat_name[i]
    }
    if (fat_name[8] != ' ') {
        out[j++] = '.'
        for (i = 8
            out[j++] = fat_name[i]
        }
    }
    out[j] = '\0'
}
static void string_to_fat_name(const char* name, uint8_t* fat_name) {
    int i = 0
    int j = 0
    memset(fat_name, ' ', 11)
    while (name[i] && name[i] != '.' && j < 8) {
        fat_name[j++] = name[i++] | 0x40
    }
    if (name[i] == '.') {
        i++
        j = 8
        while (name[i] && j < 11) {
            fat_name[j++] = name[i++] | 0x40
        }
    }
}
static int fat32_compare_name(const uint8_t* fat_name, const char* name) {
    char fat_str[13]
    fat32_name_to_string(fat_name, fat_str)
    for (int i = 0
        char c1 = fat_str[i] | 0x20
        char c2 = name[i] | 0x20
        if (c1 != c2) return 0
    }
    return 1
}
int fat32_parse_path(const char* path, char components[][FAT32_MAX_FILENAME], int max_components) {
    if (!path || !path[0]) return 0
    if (path[0] == '/') path++
    int count = 0
    int pos = 0
    while (path[pos] && count < max_components) {
        int comp_len = 0
        while (path[pos] && path[pos] != '/' && comp_len < FAT32_MAX_FILENAME - 1) {
            components[count][comp_len++] = path[pos++]
        }
        components[count][comp_len] = '\0'
        if (comp_len > 0) {
            count++
        }
        if (path[pos] == '/') pos++
    }
    return count
}
int fat32_find_entry(fat32_fs_t* fs, uint32_t dir_cluster, const char* name, 
                     fat32_dir_entry_t* entry) {
    uint8_t cluster_data[4096]
    uint32_t cluster = dir_cluster
    do {
        if (fat32_read_cluster(fs, cluster, cluster_data) != 0) {
            return -1
        }
        fat32_dir_entry_t* entries = (fat32_dir_entry_t*)cluster_data
        for (uint32_t i = 0
            if (entries[i].name[0] == 0x00) {
                return -1
            }
            if (entries[i].name[0] == 0xE5) {
                continue
            }
            if ((entries[i].attr & FAT32_ATTR_LONG_NAME) == FAT32_ATTR_LONG_NAME) {
                continue
            }
            if (fat32_compare_name(entries[i].name, name)) {
                if (entry) {
                    memcpy(entry, &entries[i], sizeof(fat32_dir_entry_t))
                }
                return 0
            }
        }
        cluster = fat32_get_next_cluster(fs, cluster)
    } while (cluster < FAT32_END_OF_CHAIN)
    return -1
}
int fat32_open(fat32_file_t* file, const char* path) {
    if (!file || !path) return -1
    char components[32][FAT32_MAX_FILENAME]
    int num_components = fat32_parse_path(path, components, 32)
    if (num_components == 0) {
        file->first_cluster = file->fs->root_cluster
        file->current_cluster = file->fs->root_cluster
        file->current_offset = 0
        file->file_size = 0
        file->is_directory = 1
        file->name[0] = '/'
        file->name[1] = '\0'
        return 0
    }
    uint32_t current_cluster = file->fs->root_cluster
    fat32_dir_entry_t entry
    for (int i = 0
        if (fat32_find_entry(file->fs, current_cluster, components[i], &entry) != 0) {
            return -1
        }
        uint32_t entry_cluster = ((uint32_t)entry.cluster_high << 16) | entry.cluster_low
        if (i == num_components - 1) {
            file->first_cluster = entry_cluster
            file->current_cluster = entry_cluster
            file->current_offset = 0
            file->file_size = entry.file_size
            file->is_directory = (entry.attr & FAT32_ATTR_DIRECTORY) ? 1 : 0
            strncpy(file->name, components[i], FAT32_MAX_FILENAME - 1)
            file->name[FAT32_MAX_FILENAME - 1] = '\0'
            return 0
        }
        if (!(entry.attr & FAT32_ATTR_DIRECTORY)) {
            return -1
        }
        current_cluster = entry_cluster
    }
    return -1
}
int fat32_read(fat32_file_t* file, void* buffer, uint32_t size) {
    if (!file || !buffer || file->is_directory) return -1
    uint8_t* buf = (uint8_t*)buffer
    uint32_t bytes_read = 0
    uint8_t cluster_data[4096]
    while (bytes_read < size && file->current_offset < file->file_size) {
        uint32_t cluster_offset = file->current_offset % file->fs->bytes_per_cluster
        if (cluster_offset == 0 || file->current_cluster != file->first_cluster) {
            if (fat32_read_cluster(file->fs, file->current_cluster, cluster_data) != 0) {
                break
            }
        }
        uint32_t bytes_left_in_cluster = file->fs->bytes_per_cluster - cluster_offset
        uint32_t bytes_left_in_file = file->file_size - file->current_offset
        uint32_t bytes_to_copy = size - bytes_read
        if (bytes_to_copy > bytes_left_in_cluster) bytes_to_copy = bytes_left_in_cluster
        if (bytes_to_copy > bytes_left_in_file) bytes_to_copy = bytes_left_in_file
        memcpy(buf + bytes_read, cluster_data + cluster_offset, bytes_to_copy)
        bytes_read += bytes_to_copy
        file->current_offset += bytes_to_copy
        if (file->current_offset % file->fs->bytes_per_cluster == 0) {
            file->current_cluster = fat32_get_next_cluster(file->fs, file->current_cluster)
            if (file->current_cluster >= FAT32_END_OF_CHAIN) {
                break
            }
        }
    }
    return bytes_read
}
int fat32_write(fat32_file_t* file, const void* buffer, uint32_t size) {
    if (!file || !buffer || file->is_directory) return -1
    const uint8_t* buf = (const uint8_t*)buffer
    uint32_t bytes_written = 0
    uint8_t cluster_data[4096]
    while (bytes_written < size) {
        uint32_t cluster_offset = file->current_offset % file->fs->bytes_per_cluster
        if (cluster_offset != 0) {
            if (fat32_read_cluster(file->fs, file->current_cluster, cluster_data) != 0) {
                break
            }
        }
        uint32_t bytes_left_in_cluster = file->fs->bytes_per_cluster - cluster_offset
        uint32_t bytes_to_copy = size - bytes_written
        if (bytes_to_copy > bytes_left_in_cluster) bytes_to_copy = bytes_left_in_cluster
        memcpy(cluster_data + cluster_offset, buf + bytes_written, bytes_to_copy)
        if (fat32_write_cluster(file->fs, file->current_cluster, cluster_data) != 0) {
            break
        }
        bytes_written += bytes_to_copy
        file->current_offset += bytes_to_copy
        if (file->current_offset > file->file_size) {
            file->file_size = file->current_offset
        }
        if (file->current_offset % file->fs->bytes_per_cluster == 0 && 
            bytes_written < size) {
            uint32_t next_cluster = fat32_get_next_cluster(file->fs, file->current_cluster)
            if (next_cluster >= FAT32_END_OF_CHAIN) {
                next_cluster = fat32_find_free_cluster(file->fs)
                if (next_cluster == 0) {
                    break
                }
                fat32_set_fat_entry(file->fs, file->current_cluster, next_cluster)
                fat32_set_fat_entry(file->fs, next_cluster, FAT32_END_OF_CHAIN)
            }
            file->current_cluster = next_cluster
        }
    }
    return bytes_written
}
void fat32_close(fat32_file_t* file) {
    (void)file
}
int fat32_list_directory(fat32_fs_t* fs, const char* path) {
    if (!fs || !fs->mounted) return -1
    uint32_t cluster = fs->root_cluster
    if (path && path[0] && strcmp(path, "/") != 0) {
        fat32_file_t dir
        dir.fs = fs
        if (fat32_open(&dir, path) != 0) {
            terminal_writestring("FAT32: Directory not found\n")
            return -1
        }
        if (!dir.is_directory) {
            terminal_writestring("FAT32: Not a directory\n")
            return -1
        }
        cluster = dir.first_cluster
    }
    uint8_t cluster_data[4096]
    char name[13]
    do {
        if (fat32_read_cluster(fs, cluster, cluster_data) != 0) {
            return -1
        }
        fat32_dir_entry_t* entries = (fat32_dir_entry_t*)cluster_data
        for (uint32_t i = 0
            if (entries[i].name[0] == 0x00) {
                return 0
            }
            if (entries[i].name[0] == 0xE5) {
                continue
            }
            if ((entries[i].attr & FAT32_ATTR_LONG_NAME) == FAT32_ATTR_LONG_NAME) {
                continue
            }
            if (entries[i].attr & FAT32_ATTR_VOLUME_ID) {
                continue
            }
            fat32_name_to_string(entries[i].name, name)
            terminal_writestring(name)
            if (entries[i].attr & FAT32_ATTR_DIRECTORY) {
                terminal_writestring("/")
            }
            terminal_writestring("  ")
        }
        cluster = fat32_get_next_cluster(fs, cluster)
    } while (cluster < FAT32_END_OF_CHAIN)
    terminal_writestring("\n")
    return 0
}
int fat32_exists(fat32_fs_t* fs, const char* path) {
    if (!fs || !fs->mounted) return 0
    fat32_file_t file
    file.fs = fs
    return (fat32_open(&file, path) == 0) ? 1 : 0
}
int fat32_create(fat32_fs_t* fs, const char* path) {
    if (!fs || !fs->mounted || !path) return -1
    char components[32][FAT32_MAX_FILENAME]
    int num_components = fat32_parse_path(path, components, 32)
    if (num_components == 0) return -1
    uint32_t parent_cluster = fs->root_cluster
    for (int i = 0
        fat32_dir_entry_t entry
        if (fat32_find_entry(fs, parent_cluster, components[i], &entry) != 0) {
            return -1
        }
        if (!(entry.attr & FAT32_ATTR_DIRECTORY)) {
            return -1
        }
        parent_cluster = ((uint32_t)entry.cluster_high << 16) | entry.cluster_low
    }
    fat32_dir_entry_t existing
    if (fat32_find_entry(fs, parent_cluster, components[num_components - 1], &existing) == 0) {
        terminal_writestring("FAT32: File already exists\n")
        return -1
    }
    uint32_t new_cluster = fat32_find_free_cluster(fs)
    if (new_cluster == 0) {
        terminal_writestring("FAT32: Disk full\n")
        return -1
    }
    fat32_set_fat_entry(fs, new_cluster, FAT32_END_OF_CHAIN)
    uint8_t cluster_data[4096]
    if (fat32_read_cluster(fs, parent_cluster, cluster_data) != 0) {
        return -1
    }
    fat32_dir_entry_t* entries = (fat32_dir_entry_t*)cluster_data
    int found_slot = 0
    for (uint32_t i = 0
        if (entries[i].name[0] == 0x00 || entries[i].name[0] == 0xE5) {
            memset(&entries[i], 0, sizeof(fat32_dir_entry_t))
            string_to_fat_name(components[num_components - 1], entries[i].name)
            entries[i].attr = FAT32_ATTR_ARCHIVE
            entries[i].cluster_high = (new_cluster >> 16) & 0xFFFF
            entries[i].cluster_low = new_cluster & 0xFFFF
            entries[i].file_size = 0
            found_slot = 1
            break
        }
    }
    if (!found_slot) {
        terminal_writestring("FAT32: Directory full\n")
        return -1
    }
    if (fat32_write_cluster(fs, parent_cluster, cluster_data) != 0) {
        return -1
    }
    return 0
}
int fat32_mkdir(fat32_fs_t* fs, const char* path) {
    if (!fs || !fs->mounted || !path) return -1
    char components[32][FAT32_MAX_FILENAME]
    int num_components = fat32_parse_path(path, components, 32)
    if (num_components == 0) return -1
    uint32_t parent_cluster = fs->root_cluster
    for (int i = 0
        fat32_dir_entry_t entry
        if (fat32_find_entry(fs, parent_cluster, components[i], &entry) != 0) {
            return -1
        }
        if (!(entry.attr & FAT32_ATTR_DIRECTORY)) {
            return -1
        }
        parent_cluster = ((uint32_t)entry.cluster_high << 16) | entry.cluster_low
    }
    fat32_dir_entry_t existing
    if (fat32_find_entry(fs, parent_cluster, components[num_components - 1], &existing) == 0) {
        terminal_writestring("FAT32: Directory already exists\n")
        return -1
    }
    uint32_t new_cluster = fat32_find_free_cluster(fs)
    if (new_cluster == 0) {
        terminal_writestring("FAT32: Disk full\n")
        return -1
    }
    fat32_set_fat_entry(fs, new_cluster, FAT32_END_OF_CHAIN)
    uint8_t cluster_data[4096]
    memset(cluster_data, 0, fs->bytes_per_cluster)
    fat32_dir_entry_t* entries = (fat32_dir_entry_t*)cluster_data
    memset(entries[0].name, ' ', 11)
    entries[0].name[0] = '.'
    entries[0].attr = FAT32_ATTR_DIRECTORY
    entries[0].cluster_high = (new_cluster >> 16) & 0xFFFF
    entries[0].cluster_low = new_cluster & 0xFFFF
    entries[0].file_size = 0
    memset(entries[1].name, ' ', 11)
    entries[1].name[0] = '.'
    entries[1].name[1] = '.'
    entries[1].attr = FAT32_ATTR_DIRECTORY
    entries[1].cluster_high = (parent_cluster >> 16) & 0xFFFF
    entries[1].cluster_low = parent_cluster & 0xFFFF
    entries[1].file_size = 0
    if (fat32_write_cluster(fs, new_cluster, cluster_data) != 0) {
        return -1
    }
    if (fat32_read_cluster(fs, parent_cluster, cluster_data) != 0) {
        return -1
    }
    int found_slot = 0
    for (uint32_t i = 0
        if (entries[i].name[0] == 0x00 || entries[i].name[0] == 0xE5) {
            memset(&entries[i], 0, sizeof(fat32_dir_entry_t))
            string_to_fat_name(components[num_components - 1], entries[i].name)
            entries[i].attr = FAT32_ATTR_DIRECTORY
            entries[i].cluster_high = (new_cluster >> 16) & 0xFFFF
            entries[i].cluster_low = new_cluster & 0xFFFF
            entries[i].file_size = 0
            found_slot = 1
            break
        }
    }
    if (!found_slot) {
        terminal_writestring("FAT32: Directory full\n")
        return -1
    }
    if (fat32_write_cluster(fs, parent_cluster, cluster_data) != 0) {
        return -1
    }
    return 0
}
int fat32_delete(fat32_fs_t* fs, const char* path) {
    if (!fs || !fs->mounted || !path) return -1
    char components[32][FAT32_MAX_FILENAME]
    int num_components = fat32_parse_path(path, components, 32)
    if (num_components == 0) return -1
    uint32_t parent_cluster = fs->root_cluster
    for (int i = 0
        fat32_dir_entry_t entry
        if (fat32_find_entry(fs, parent_cluster, components[i], &entry) != 0) {
            return -1
        }
        if (!(entry.attr & FAT32_ATTR_DIRECTORY)) {
            return -1
        }
        parent_cluster = ((uint32_t)entry.cluster_high << 16) | entry.cluster_low
    }
    uint8_t cluster_data[4096]
    uint32_t cluster = parent_cluster
    int found = 0
    uint32_t file_cluster = 0
    do {
        if (fat32_read_cluster(fs, cluster, cluster_data) != 0) {
            return -1
        }
        fat32_dir_entry_t* entries = (fat32_dir_entry_t*)cluster_data
        for (uint32_t i = 0
            if (entries[i].name[0] == 0x00) {
                return -1
            }
            if (entries[i].name[0] == 0xE5) continue
            if ((entries[i].attr & FAT32_ATTR_LONG_NAME) == FAT32_ATTR_LONG_NAME) continue
            if (fat32_compare_name(entries[i].name, components[num_components - 1])) {
                if (entries[i].attr & FAT32_ATTR_DIRECTORY) {
                    terminal_writestring("FAT32: Cannot delete directory\n")
                    return -1
                }
                file_cluster = ((uint32_t)entries[i].cluster_high << 16) | entries[i].cluster_low
                entries[i].name[0] = 0xE5
                found = 1
                break
            }
        }
        if (found) {
            if (fat32_write_cluster(fs, cluster, cluster_data) != 0) {
                return -1
            }
            break
        }
        cluster = fat32_get_next_cluster(fs, cluster)
    } while (cluster < FAT32_END_OF_CHAIN)
    if (!found) {
        terminal_writestring("FAT32: File not found\n")
        return -1
    }
    while (file_cluster && file_cluster < FAT32_END_OF_CHAIN) {
        uint32_t next = fat32_get_next_cluster(fs, file_cluster)
        fat32_set_fat_entry(fs, file_cluster, FAT32_FREE_CLUSTER)
        file_cluster = next
    }
    return 0
}
int fat32_get_file_size(fat32_fs_t* fs, const char* path) {
    if (!fs || !fs->mounted || !path) return -1
    fat32_file_t file
    file.fs = fs
    if (fat32_open(&file, path) != 0) {
        return -1
    }
    return (int)file.file_size
}
uint32_t fat32_read_cluster_chain(fat32_fs_t* fs, uint32_t start_cluster, 
                                   uint8_t* buffer, uint32_t max_bytes) {
    if (!fs || !buffer || start_cluster < 2) return 0
    uint32_t bytes_read = 0
    uint32_t cluster = start_cluster
    while (cluster < FAT32_END_OF_CHAIN && bytes_read < max_bytes) {
        uint32_t to_read = fs->bytes_per_cluster
        if (bytes_read + to_read > max_bytes) {
            to_read = max_bytes - bytes_read
        }
        if (fat32_read_cluster(fs, cluster, buffer + bytes_read) != 0) {
            break
        }
        bytes_read += to_read
        cluster = fat32_get_next_cluster(fs, cluster)
    }
    return bytes_read
}