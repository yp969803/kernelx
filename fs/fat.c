#include "fat.h"
#include "../drivers/ata.h"
#include "../kernel/kmalloc.h"
#include "../kernel/utils.h"

fat_boot_sector_t hda_boot_sector;

static void cal_fat_layout(uint32_t sectors, uint8_t *sec_per_clus, uint16_t *fat_size)
{
    uint8_t sectors_per_cluser = 1;
    uint16_t max_clusters      = 65524; // Maximum number of clusters for FAT16
    uint16_t clusters          = sectors * SECTOR_SIZE / (4 + sectors_per_cluser * SECTOR_SIZE);

    while (clusters > max_clusters) {
        sectors_per_cluser *= 2;
        clusters = sectors * SECTOR_SIZE / (4 + sectors_per_cluser * SECTOR_SIZE);
    }
    *sec_per_clus = sectors_per_cluser;
    *fat_size     = clusters * 2 / SECTOR_SIZE;
}

int mkfs_fat(void)
{
    uint32_t total_sectors                = disk.size_in_sectors;
    hda_boot_sector.bytes_per_sector      = SECTOR_SIZE;
    hda_boot_sector.reserved_sector_count = 1;
    hda_boot_sector.num_fats              = FAT_COPIES;
    hda_boot_sector.root_entry_count      = ROOT_ENTRYS;

    uint16_t root_dir_sectors = CEIL_DIV(ROOT_ENTRYS * 32, SECTOR_SIZE);
    cal_fat_layout(total_sectors - hda_boot_sector.reserved_sector_count - root_dir_sectors,
                   &hda_boot_sector.sectors_per_cluster, &hda_boot_sector.fat_size_16);

    hda_boot_sector.total_sectors_32 = total_sectors;

    if (ata_write_sectors(0, 1, (const uint8_t *)&hda_boot_sector) != OK) {
        return ERR;
    }

    uint16_t *fat_kmalloc = kmalloc(hda_boot_sector.fat_size_16 * SECTOR_SIZE);
    if (!fat_kmalloc) {
        return ERR;
    }

    for (uint32_t i = 0; i < (hda_boot_sector.fat_size_16 * SECTOR_SIZE) / 2; i++) {
        fat_kmalloc[i] = FREE_CLUSTER;
    }
    fat_kmalloc[0] = RESERVED_CLUSTER;
    fat_kmalloc[1] = END_OF_CLUSTER_CHAIN;

    for (uint32_t i = 0; i < FAT_COPIES; i++) {
        if (ata_write_sectors(hda_boot_sector.reserved_sector_count +
                                  i * hda_boot_sector.fat_size_16,
                              hda_boot_sector.fat_size_16, (const uint8_t *)fat_kmalloc) != OK) {
            return ERR;
        }
    }
    return OK;
}

int read_fat_boot_sector(void)
{
    return ata_read_sectors(0, 1, (uint8_t *)&hda_boot_sector);
}