typedef struct {
    uint8_t  jmp_boot[3]
    uint8_t  oem_name[8]
    uint16_t bytes_per_sector
    uint8_t  sectors_per_cluster
    uint16_t reserved_sector_count
    uint8_t  num_fats
    uint16_t root_entry_count
    uint16_t total_sectors_16
    uint8_t  media_type
    uint16_t fat_size_16
    uint16_t sectors_per_track
    uint16_t num_heads
    uint32_t hidden_sectors
    uint32_t total_sectors_32
    uint32_t fat_size_32
    uint16_t ext_flags
    uint16_t fs_version
    uint32_t root_cluster
    uint16_t fs_info_sector
    uint16_t backup_boot_sector
    uint8_t  reserved[12]
    uint8_t  drive_num
    uint8_t  reserved1
    uint8_t  boot_signature
    uint32_t volume_id
    uint8_t  volume_label[11]
    uint8_t  fs_type[8]
} __attribute__((packed)) fat32_boot_sector_t
typedef struct {
    uint8_t  name[11]
    uint8_t  attr
    uint8_t  reserved
    uint8_t  create_time_tenth
    uint16_t create_time
    uint16_t create_date
    uint16_t last_access_date
    uint16_t cluster_high
    uint16_t modify_time
    uint16_t modify_date
    uint16_t cluster_low
    uint32_t file_size
} __attribute__((packed)) fat32_dir_entry_t
typedef struct {
    uint8_t  order
    uint16_t name1[5]
    uint8_t  attr
    uint8_t  type
    uint8_t  checksum
    uint16_t name2[6]
    uint16_t reserved
    uint16_t name3[2]
} __attribute__((packed)) fat32_lfn_entry_t
typedef struct {
    uint8_t drive
    uint32_t partition_start
    uint32_t fat_start
    uint32_t data_start
    uint32_t root_cluster
    uint32_t bytes_per_sector
    uint32_t sectors_per_cluster
    uint32_t bytes_per_cluster
    uint32_t fat_size
    uint32_t total_clusters
    uint16_t reserved_sector_count
    uint8_t num_fats
    uint8_t mounted
    char mount_point[64]
} fat32_fs_t
typedef struct {
    fat32_fs_t* fs
    uint32_t first_cluster
    uint32_t current_cluster
    uint32_t current_offset
    uint32_t file_size
    uint8_t is_directory
    char name[FAT32_MAX_FILENAME]
} fat32_file_t
void fat32_init(void)
int fat32_mount(uint8_t drive, uint32_t partition_start, const char* mount_point)
int fat32_unmount(const char* mount_point)
int fat32_open(fat32_file_t* file, const char* path)
int fat32_read(fat32_file_t* file, void* buffer, uint32_t size)
int fat32_write(fat32_file_t* file, const void* buffer, uint32_t size)
void fat32_close(fat32_file_t* file)
int fat32_list_directory(fat32_fs_t* fs, const char* path)
int fat32_exists(fat32_fs_t* fs, const char* path)
int fat32_create(fat32_fs_t* fs, const char* path)
int fat32_mkdir(fat32_fs_t* fs, const char* path)
int fat32_delete(fat32_fs_t* fs, const char* path)
int fat32_get_file_size(fat32_fs_t* fs, const char* path)
fat32_fs_t* fat32_get_mounted_fs(const char* mount_point)
fat32_fs_t* fat32_get_mounted_fs_by_index(int index)
uint32_t fat32_read_cluster_chain(fat32_fs_t* fs, uint32_t start_cluster, 
                                   uint8_t* buffer, uint32_t max_bytes)
uint32_t fat32_get_next_cluster(fat32_fs_t* fs, uint32_t cluster)
int fat32_find_entry(fat32_fs_t* fs, uint32_t dir_cluster, const char* name, 
                     fat32_dir_entry_t* entry)
int fat32_parse_path(const char* path, char components[][FAT32_MAX_FILENAME], int max_components)
