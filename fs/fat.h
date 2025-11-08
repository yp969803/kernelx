#pragma once

#include <stdint.h>

#define SECTOR_SIZE 512
// #define CLUSTER_SIZE 1024
#define ROOT_ENTRYS 512
#define FAT_COPIES 2

#define FREE_CLUSTER 0x0000
#define END_OF_CLUSTER_CHAIN 0xFFFF
#define RESERVED_CLUSTER 0xFFF0
#define BAD_CLUSTER 0xFFF7

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