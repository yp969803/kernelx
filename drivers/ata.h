#pragma once

#include <stdint.h>

#define ATA_PRIMARY_IO 0x1F0
#define ATA_SECONDARY_IO 0x170
#define ATA_PRIMARY_CTRL 0x3F6
#define ATA_SECONDARY_CTRL 0x376
#define ATA_SECTOR_SIZE 512

#define ATA_SR_BSY 0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DF 0x20
#define ATA_SR_DRQ 0x08
#define ATA_SR_ERR 0x01

#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_WRITE_PIO 0x30

typedef struct {
    uint16_t io_base;
    uint16_t ctrl_base;
    uint8_t slave;            // 0 for master, 1 for slave
    uint32_t size_in_sectors; // total size of the partition in sectors
    uint32_t partition_start; // sector number where partition starts
} ATA_Device;