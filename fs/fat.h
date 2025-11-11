#pragma once

#include <stdint.h>

#define SECTOR_SIZE 512
// #define CLUSTER_SIZE 1024
#define ROOT_ENTRYS 512
#define FAT_COPIES 2

#define FREE_CLUSTER 0x0000
#define LAST_CLUSTER 0xFFEF
#define END_OF_CLUSTER_CHAIN 0xFFF8
#define RESERVED_CLUSTER 0xFFF0
#define BAD_CLUSTER 0xFFF7

#define ROOT_CLUSTER 0
#define DIR_ENTRY_RM 0XE5

typedef struct {
    uint8_t jump_boot[3];
    char oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count; // 1 for FAT12/16
    uint8_t num_fats;               // 2 for FAT12/16
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t media; // 0xF8 for fixed drives, 0xF0 for removable drives
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature; // 0x29 indicates next three fields are present
    uint32_t volume_id;
    char volume_label[11];
    char file_system_type[8];
} __attribute__((packed)) fat_boot_sector_t;

#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20

typedef struct {
    char name[11];
    uint8_t attr;
    uint8_t reserved;
    uint8_t create_time_tenths;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} __attribute__((packed)) fat_directory_entry_t;

int mkfs_fat(void);
int mkdir_fat(const char *name);
int read_fat_boot_sector(void);
fat_directory_entry_t *fat_get_dir_entry(uint16_t cluster, const char name[11]);
uint16_t *get_fat_structure(void);
int fat_set_dir_entry(uint16_t cluster, fat_directory_entry_t *entry, uint16_t *fat_table);
uint16_t fat_return_free_cluster(uint16_t *fat_table);
uint16_t fat_return_last_cluster(uint16_t *fat_table, uint16_t start_cluster);
int fat_write_fat_table(uint16_t *fat_table);
int fat_rm_entry(const char *name);
int mkfile_fat(const char *name);