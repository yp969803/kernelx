#include "fat.h"
#include "../drivers/ata.h"
#include "../kernel/kmalloc.h"
#include "../kernel/mem.h"
#include "../kernel/utils.h"
#include "../stdlib/string.h"
#include <stdbool.h>

fat_boot_sector_t hda_boot_sector;

bool dot_only_in_last_entry(const char *name)
{

    int i = 0;

    int slash_pos = -1;
    int dot_pos   = -1;
    int dot_count = 0;

    while (name[i] != '\0') {
        if (name[i] == '.') {
            dot_pos = i;
            if (name[i + 1] == '\0') {
                return false;
            }
            dot_count++;
        }
        if (name[i] == '/') {
            slash_pos = i;
        }

        i++;
    }

    if (dot_count == 0) {
        return true;
    }

    if (dot_count > 1) {
        return false;
    }

    if (dot_pos < slash_pos) {
        return false;
    }

    return true;
}

void rearrange_name(char *name)
{
    char ext[3] = {' ', ' ', ' '};
    for (int i = 0; i < 11; i++) {
        if (name[i] == '.') {
            int j = 0;
            i++;
            while (i < 11 && j < 3) {
                ext[j]  = name[i];
                name[i] = ' ';
                i++;
                j++;
            }
            break;
        }
    }
    for (int i = 8, j = 0; j < 3; i++, j++) {
        name[i] = ext[j];
    }
}

static void cal_fat_layout(uint32_t sectors, uint8_t *sec_per_clus, uint16_t *fat_size)
{
    uint8_t sectors_per_cluster = 1;
    uint16_t max_clusters       = 65024; // Maximum number of clusters for FAT16
    uint16_t fat_sectors;
    uint32_t clusters_count;

    while (1) {
        clusters_count = sectors / sectors_per_cluster;
        fat_sectors    = clusters_count * 2 / SECTOR_SIZE;
        sectors        = sectors - (FAT_COPIES * fat_sectors);
        if (clusters_count <= max_clusters) {
            break;
        }
        sectors_per_cluster *= 2;

        if (sectors_per_cluster > 128) {
            // Exceeded maximum cluster size for FAT16
            break;
        }
    }
    *sec_per_clus = sectors_per_cluster;
    *fat_size     = fat_sectors;
}

int mkfs_fat(void)
{
    uint32_t total_sectors                = disk.size_in_sectors;
    hda_boot_sector.bytes_per_sector      = SECTOR_SIZE;
    hda_boot_sector.reserved_sector_count = 1;
    hda_boot_sector.num_fats              = FAT_COPIES;
    hda_boot_sector.root_entry_count      = ROOT_ENTRYS;

    uint16_t root_dir_sectors = CEIL_DIV(ROOT_ENTRYS * 32, SECTOR_SIZE);

    uint8_t sectors_per_cluster = hda_boot_sector.sectors_per_cluster;
    uint16_t fat_size_16        = hda_boot_sector.fat_size_16;

    cal_fat_layout(total_sectors - hda_boot_sector.reserved_sector_count - root_dir_sectors,
                   &sectors_per_cluster, &fat_size_16);

    hda_boot_sector.sectors_per_cluster = sectors_per_cluster;
    hda_boot_sector.fat_size_16         = fat_size_16;

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
        if (ata_write_sectors(
                hda_boot_sector.reserved_sector_count + i * hda_boot_sector.fat_size_16,
                (uint8_t)hda_boot_sector.fat_size_16, (const uint8_t *)fat_kmalloc) != OK) {
            return ERR;
        }
    }
    return OK;
}

int read_fat_boot_sector(void)
{
    return ata_read_sectors(0, 1, (uint8_t *)&hda_boot_sector);
}

int mkdir_fat(const char *name)
{
    char dir_name[11];
    mem_set(dir_name, ' ', 11);

    uint16_t cluster = ROOT_CLUSTER;

    fat_directory_entry_t *existing_entry = fat_lookup(name, dir_name, &cluster);

    if (existing_entry) {
        kfree(existing_entry);
        return ERR;
    }

    fat_directory_entry_t new_entry;
    mem_set(&new_entry, 0, sizeof(fat_directory_entry_t));
    new_entry.attr = ATTR_DIRECTORY;
    mem_copy(dir_name, new_entry.name, 11);

    uint16_t *fat_table = get_fat_structure();
    if (!fat_table) {
        return ERR;
    }

    uint16_t new_cluster = fat_return_free_cluster(fat_table);
    if (new_cluster == 0) {
        kfree(fat_table);
        return ERR;
    }

    uint16_t new_cluster_sector = hda_boot_sector.reserved_sector_count +
                                  hda_boot_sector.num_fats * hda_boot_sector.fat_size_16 +
                                  CEIL_DIV(hda_boot_sector.root_entry_count * 32, SECTOR_SIZE) +
                                  (new_cluster - 2) * hda_boot_sector.sectors_per_cluster;

    fat_directory_entry_t *dir_entries = kmalloc(hda_boot_sector.sectors_per_cluster * SECTOR_SIZE);
    mem_set(dir_entries, 0, hda_boot_sector.sectors_per_cluster * SECTOR_SIZE);

    uint32_t *dir_entries_u32 = (uint32_t *)dir_entries;

    dir_entries_u32[0] = new_cluster; // "." entry
    dir_entries_u32[1] = cluster;     // ".." entry

    if (ata_write_sectors(new_cluster_sector, hda_boot_sector.sectors_per_cluster,
                          (const uint8_t *)dir_entries) != OK) {
        kfree(dir_entries);
        kfree(fat_table);
        return ERR;
    }

    fat_table[new_cluster]      = END_OF_CLUSTER_CHAIN;
    new_entry.first_cluster_low = new_cluster;

    if (fat_set_dir_entry(cluster, &new_entry, fat_table) != OK) {
        kfree(fat_table);
        return ERR;
    }
    if (fat_write_fat_table(fat_table) != OK) {
        kfree(fat_table);
        return ERR;
    }
    kfree(fat_table);
    return OK;
}

fat_directory_entry_t *fat_get_dir_entry(uint16_t cluster, const char name[11])
{
    if (cluster == ROOT_CLUSTER) {
        uint8_t root_dir_sectors = CEIL_DIV(hda_boot_sector.root_entry_count * 32, SECTOR_SIZE);
        uint32_t root_dir_start_sector = hda_boot_sector.reserved_sector_count +
                                         hda_boot_sector.num_fats * hda_boot_sector.fat_size_16;
        uint32_t total_entries = hda_boot_sector.root_entry_count;

        fat_directory_entry_t *root_dir_entries = kmalloc(root_dir_sectors * SECTOR_SIZE);
        if (!root_dir_entries) {
            return NULL;
        }

        if (ata_read_sectors(root_dir_start_sector, root_dir_sectors,
                             (uint8_t *)root_dir_entries) != OK) {
            kfree(root_dir_entries);
            return NULL;
        }

        for (uint32_t i = 0; i < total_entries; i++) {
            uint8_t first_byte = root_dir_entries[i].name[0];
            if (first_byte == 0x00) {
                break;
            }
            if (first_byte == DIR_ENTRY_RM) {
                continue;
            }
            if (streq(root_dir_entries[i].name, name, 11)) {
                fat_directory_entry_t *result = kmalloc(sizeof(fat_directory_entry_t));
                if (result) {
                    mem_copy(&root_dir_entries[i], result, sizeof(fat_directory_entry_t));
                }
                kfree(root_dir_entries);
                return result;
            }
        }
        kfree(root_dir_entries);
        return NULL;
    }

    if (cluster == 1 || cluster > LAST_CLUSTER) {
        return NULL;
    }

    uint16_t *fat_table = get_fat_structure();
    if (!fat_table) {
        return NULL;
    }

    uint16_t current        = cluster;
    uint32_t total_clusters = 0;

    while (current >= 0x0002 && current <= LAST_CLUSTER) {
        total_clusters++;
        current = fat_table[current];
    }

    uint32_t total_sectors = total_clusters * hda_boot_sector.sectors_per_cluster;

    uint32_t first_data_sector = hda_boot_sector.reserved_sector_count +
                                 hda_boot_sector.num_fats * hda_boot_sector.fat_size_16 +
                                 CEIL_DIV(hda_boot_sector.root_entry_count * 32, SECTOR_SIZE);

    fat_directory_entry_t *dir_entries = kmalloc(total_sectors * SECTOR_SIZE);

    if (!dir_entries) {
        kfree(fat_table);
        return NULL;
    }

    uint8_t *buffer = (uint8_t *)dir_entries;

    current = cluster;
    while (current >= 0x0002 && current < 0xFFF8) {
        uint32_t start_sector =
            first_data_sector + (current - 2) * hda_boot_sector.sectors_per_cluster;
        if (ata_read_sectors(start_sector, hda_boot_sector.sectors_per_cluster,
                             (uint8_t *)buffer) != OK) {
            kfree(dir_entries);
            kfree(fat_table);
            return NULL;
        }
        buffer += hda_boot_sector.sectors_per_cluster * SECTOR_SIZE;
        current = fat_table[current];
    }

    kfree(fat_table);

    for (uint32_t i = 2; i < (total_sectors * SECTOR_SIZE) / sizeof(fat_directory_entry_t); i++) {
        uint8_t first_byte = dir_entries[i].name[0];
        if (first_byte == 0x00) {
            break;
        }
        if (first_byte == DIR_ENTRY_RM) {
            continue;
        }
        if (streq(dir_entries[i].name, name, 11)) {
            fat_directory_entry_t *result = kmalloc(sizeof(fat_directory_entry_t));
            if (result) {
                mem_copy(&dir_entries[i], result, sizeof(fat_directory_entry_t));
            }
            kfree(dir_entries);
            return result;
        }
    }
    kfree(dir_entries);
    return NULL;
}

uint16_t *get_fat_structure(void)
{
    uint16_t *fat_table = kmalloc(hda_boot_sector.fat_size_16 * SECTOR_SIZE);
    if (!fat_table) {
        return NULL;
    }

    if (ata_read_sectors(hda_boot_sector.reserved_sector_count,
                         (uint8_t)hda_boot_sector.fat_size_16, (uint8_t *)fat_table) != OK) {
        kfree(fat_table);
        return NULL;
    }
    return fat_table;
}

int fat_set_dir_entry(uint16_t cluster, fat_directory_entry_t *entry, uint16_t *fat_table)
{
    if (fat_table == NULL || entry == NULL) {
        return ERR;
    }

    if (cluster == ROOT_CLUSTER) {
        uint8_t root_dir_sectors = CEIL_DIV(hda_boot_sector.root_entry_count * 32, SECTOR_SIZE);
        uint32_t root_dir_start_sector = hda_boot_sector.reserved_sector_count +
                                         hda_boot_sector.num_fats * hda_boot_sector.fat_size_16;
        uint32_t total_entries = hda_boot_sector.root_entry_count;

        fat_directory_entry_t *root_dir_entries = kmalloc(root_dir_sectors * SECTOR_SIZE);
        if (!root_dir_entries) {
            return ERR;
        }

        if (ata_read_sectors(root_dir_start_sector, root_dir_sectors,
                             (uint8_t *)root_dir_entries) != OK) {
            kfree(root_dir_entries);
            return ERR;
        }

        bool free_slot_found = false;

        for (uint32_t i = 0; i < total_entries; i++) {
            uint8_t first_byte = root_dir_entries[i].name[0];

            if (first_byte != 0x00 && first_byte != DIR_ENTRY_RM) {
                continue;
            }
            mem_copy(entry, &root_dir_entries[i], sizeof(fat_directory_entry_t));
            free_slot_found = true;
            break;
        }

        if (!free_slot_found) {
            kfree(root_dir_entries);
            return ERR;
        }

        if (ata_write_sectors(root_dir_start_sector, root_dir_sectors,
                              (const uint8_t *)root_dir_entries) != OK) {
            kfree(root_dir_entries);
            return ERR;
        }

        kfree(root_dir_entries);
        return OK;
    }

    if (cluster == 1 || cluster > LAST_CLUSTER) {
        return ERR;
    }

    uint32_t first_data_sector = hda_boot_sector.reserved_sector_count +
                                 hda_boot_sector.num_fats * hda_boot_sector.fat_size_16 +
                                 CEIL_DIV(hda_boot_sector.root_entry_count * 32, SECTOR_SIZE);

    uint16_t current = cluster;

    bool first_iteration = true;

    while (current >= 0x0002 && current <= LAST_CLUSTER) {

        uint32_t start_sector =
            first_data_sector + (current - 2) * hda_boot_sector.sectors_per_cluster;
        fat_directory_entry_t *dir_entries =
            kmalloc(hda_boot_sector.sectors_per_cluster * SECTOR_SIZE);
        if (!dir_entries) {
            return ERR;
        }
        if (ata_read_sectors(start_sector, hda_boot_sector.sectors_per_cluster,
                             (uint8_t *)dir_entries) != OK) {
            kfree(dir_entries);
            return ERR;
        }

        bool free_slot_found = false;

        uint32_t i = 0;
        if (first_iteration) {
            i               = 2; // Skip "." and ".." entries
            first_iteration = false;
        }

        for (; i <
               (hda_boot_sector.sectors_per_cluster * SECTOR_SIZE) / sizeof(fat_directory_entry_t);
             i++) {
            uint8_t first_byte = dir_entries[i].name[0];

            if (first_byte != 0x00 && first_byte != DIR_ENTRY_RM) {
                continue;
            }
            mem_copy(entry, &dir_entries[i], sizeof(fat_directory_entry_t));
            free_slot_found = true;
            break;
        }
        if (free_slot_found) {
            if (ata_write_sectors(start_sector, hda_boot_sector.sectors_per_cluster,
                                  (const uint8_t *)dir_entries) != OK) {
                kfree(dir_entries);
                return ERR;
            }
            kfree(dir_entries);
            return OK;
        }
        current = fat_table[current];
    }

    uint16_t new_cluster  = fat_return_free_cluster(fat_table);
    uint16_t last_cluster = fat_return_last_cluster(fat_table, cluster);

    if (new_cluster == 0 || last_cluster == 0) {
        return ERR;
    }

    fat_table[last_cluster] = new_cluster;
    fat_table[new_cluster]  = END_OF_CLUSTER_CHAIN;

    uint32_t start_sector =
        first_data_sector + (new_cluster - 2) * hda_boot_sector.sectors_per_cluster;

    fat_directory_entry_t *dir_entries = kmalloc(hda_boot_sector.sectors_per_cluster * SECTOR_SIZE);
    mem_set(dir_entries, 0, hda_boot_sector.sectors_per_cluster * SECTOR_SIZE);

    dir_entries[0] = *entry;

    if (ata_write_sectors(start_sector, hda_boot_sector.sectors_per_cluster,
                          (const uint8_t *)dir_entries) != OK) {
        kfree(dir_entries);
        kfree(fat_table);
        return ERR;
    }

    kfree(dir_entries);
    return OK;
}

int fat_rm_dir_entry(uint16_t cluster, fat_directory_entry_t *entry)
{
    if (entry == NULL) {
        return ERR;
    }

    if (cluster == ROOT_CLUSTER) {
        uint8_t root_dir_sectors = CEIL_DIV(hda_boot_sector.root_entry_count * 32, SECTOR_SIZE);

        uint32_t root_dir_start_sector = hda_boot_sector.reserved_sector_count +
                                         hda_boot_sector.num_fats * hda_boot_sector.fat_size_16;

        uint32_t total_entries = hda_boot_sector.root_entry_count;

        fat_directory_entry_t *root_dir_entries = kmalloc(root_dir_sectors * SECTOR_SIZE);
        if (!root_dir_entries) {
            return ERR;
        }

        if (ata_read_sectors(root_dir_start_sector, root_dir_sectors,
                             (uint8_t *)root_dir_entries) != OK) {
            kfree(root_dir_entries);
            return ERR;
        }

        bool slot_found = false;

        for (uint32_t i = 0; i < total_entries; i++) {
            uint8_t first_byte = root_dir_entries[i].name[0];

            if (first_byte == 0x00) {
                kfree(root_dir_entries);
                return ERR;
            }

            if (first_byte == DIR_ENTRY_RM) {
                continue;
            }

            if (streq(root_dir_entries[i].name, entry->name, 11)) {
                root_dir_entries[i].name[0] = DIR_ENTRY_RM;
                slot_found                  = true;
                break;
            }
        }

        if (!slot_found) {
            kfree(root_dir_entries);
            return ERR;
        }

        if (ata_write_sectors(root_dir_start_sector, root_dir_sectors,
                              (const uint8_t *)root_dir_entries) != OK) {
            kfree(root_dir_entries);
            return ERR;
        }

        kfree(root_dir_entries);
        return OK;
    }

    if (cluster == 1 || cluster > LAST_CLUSTER) {
        return ERR;
    }

    uint32_t first_data_sector = hda_boot_sector.reserved_sector_count +
                                 hda_boot_sector.num_fats * hda_boot_sector.fat_size_16 +
                                 CEIL_DIV(hda_boot_sector.root_entry_count * 32, SECTOR_SIZE);

    uint16_t current = cluster;

    bool first_iteration = true;

    while (current >= 0x0002 && current <= LAST_CLUSTER) {

        uint32_t start_sector =
            first_data_sector + (current - 2) * hda_boot_sector.sectors_per_cluster;
        fat_directory_entry_t *dir_entries =
            kmalloc(hda_boot_sector.sectors_per_cluster * SECTOR_SIZE);
        if (!dir_entries) {
            return ERR;
        }
        if (ata_read_sectors(start_sector, hda_boot_sector.sectors_per_cluster,
                             (uint8_t *)dir_entries) != OK) {
            kfree(dir_entries);
            return ERR;
        }

        bool slot_found = false;

        uint32_t i = 0;
        if (first_iteration) {
            i               = 2; // Skip "." and ".." entries
            first_iteration = false;
        }

        for (; i <
               (hda_boot_sector.sectors_per_cluster * SECTOR_SIZE) / sizeof(fat_directory_entry_t);
             i++) {
            uint8_t first_byte = dir_entries[i].name[0];

            if (first_byte == 0x00) {
                kfree(dir_entries);
                return ERR;
            }

            if (first_byte == DIR_ENTRY_RM) {
                continue;
            }
            if (streq(dir_entries[i].name, entry->name, 11)) {
                dir_entries[i].name[0] = DIR_ENTRY_RM;
                slot_found             = true;
                break;
            }
        }
        if (slot_found) {
            if (ata_write_sectors(start_sector, hda_boot_sector.sectors_per_cluster,
                                  (const uint8_t *)dir_entries) != OK) {
                kfree(dir_entries);
                return ERR;
            }
            kfree(dir_entries);
            return OK;
        }
    }

    return ERR;
}

uint16_t fat_return_free_cluster(uint16_t *fat_table)
{
    if (!fat_table) {
        return 0;
    }

    for (uint16_t i = 2; i < (hda_boot_sector.fat_size_16 * SECTOR_SIZE) / 2; i++) {
        if (fat_table[i] == FREE_CLUSTER) {
            return i;
        }
    }
    return 0;
}

uint16_t fat_return_last_cluster(uint16_t *fat_table, uint16_t start_cluster)
{
    if (!fat_table) {
        return 0;
    }

    uint16_t current = start_cluster;

    while (current >= 0x0002 && current < END_OF_CLUSTER_CHAIN) {
        if (fat_table[current] >= END_OF_CLUSTER_CHAIN) {
            return current;
        }
        current = fat_table[current];
    }
    return 0;
}

int fat_write_fat_table(uint16_t *fat_table)
{
    if (!fat_table) {
        return ERR;
    }

    for (uint32_t i = 0; i < FAT_COPIES; i++) {
        if (ata_write_sectors(
                hda_boot_sector.reserved_sector_count + i * hda_boot_sector.fat_size_16,
                (uint8_t)hda_boot_sector.fat_size_16, (const uint8_t *)fat_table) != OK) {
            return ERR;
        }
    }
    return OK;
}

void fat_free_fat_table_clusters(uint16_t start_cluster, uint16_t *fat_table)
{
    if (!fat_table || start_cluster < 2 || start_cluster > LAST_CLUSTER) {
        return;
    }

    uint16_t current = start_cluster;

    while (current >= 0x0002 && current < END_OF_CLUSTER_CHAIN) {
        uint16_t next      = fat_table[current];
        fat_table[current] = FREE_CLUSTER;
        current            = next;
    }
}

int fat_rm_entry(const char *name)
{
    char dir_name[11];
    mem_set(dir_name, ' ', 11);

    uint16_t cluster = ROOT_CLUSTER;

    fat_directory_entry_t *existing_entry = fat_lookup(name, dir_name, &cluster);
    if (!existing_entry) {
        return ERR;
    }

    uint16_t *fat_table = get_fat_structure();
    if (!fat_table) {
        return ERR;
    }

    fat_free_fat_table_clusters(existing_entry->first_cluster_low, fat_table);

    if (fat_rm_dir_entry(cluster, existing_entry) != OK || fat_write_fat_table(fat_table) != OK) {
        kfree(existing_entry);
        kfree(fat_table);
        return ERR;
    }

    kfree(existing_entry);
    kfree(fat_table);
    return OK;
}

int mkfile_fat(const char *name)
{
    char dir_name[11];
    mem_set(dir_name, ' ', 11);

    uint16_t cluster = ROOT_CLUSTER;

    fat_directory_entry_t *existing_entry = fat_lookup(name, dir_name, &cluster);
    if (existing_entry) {
        kfree(existing_entry);
        return ERR;
    }

    fat_directory_entry_t new_entry;
    mem_set(&new_entry, 0, sizeof(fat_directory_entry_t));
    new_entry.attr = ATTR_ARCHIVE;
    mem_copy(dir_name, new_entry.name, 11);
    new_entry.first_cluster_low = 0;
    new_entry.file_size         = 0;

    uint16_t *fat_table = get_fat_structure();
    if (!fat_table) {
        return ERR;
    }

    if (fat_set_dir_entry(cluster, &new_entry, fat_table) != OK) {
        kfree(fat_table);
        return ERR;
    }

    if (fat_write_fat_table(fat_table) != OK) {
        kfree(fat_table);
        return ERR;
    }

    kfree(fat_table);
    return OK;
}

int fat_read_file(uint16_t start_cluster, uint8_t *buffer, uint32_t size)
{
    if (start_cluster < 2 || start_cluster > LAST_CLUSTER || !buffer || size == 0) {
        return ERR;
    }

    uint16_t *fat_table = get_fat_structure();
    if (!fat_table) {
        return ERR;
    }

    uint32_t first_data_sector = hda_boot_sector.reserved_sector_count +
                                 hda_boot_sector.num_fats * hda_boot_sector.fat_size_16 +
                                 CEIL_DIV(hda_boot_sector.root_entry_count * 32, SECTOR_SIZE);

    uint16_t current    = start_cluster;
    uint32_t bytes_read = 0;
    uint8_t *buf_ptr    = buffer;

    while (current >= 0x0002 && current < END_OF_CLUSTER_CHAIN && bytes_read < size) {
        uint32_t start_sector =
            first_data_sector + (current - 2) * hda_boot_sector.sectors_per_cluster;

        uint32_t cluster_size = hda_boot_sector.sectors_per_cluster * SECTOR_SIZE;
        uint32_t to_read      = MIN(cluster_size, size - bytes_read);

        if (ata_read_sectors(start_sector, hda_boot_sector.sectors_per_cluster, buf_ptr) != OK) {
            kfree(fat_table);
            return ERR;
        }

        buf_ptr += to_read;
        bytes_read += to_read;
        current = fat_table[current];
    }

    kfree(fat_table);
    return OK;
}

//  requires full path
fat_directory_entry_t *fat_lookup(const char *name, char *dir_name, uint16_t *cluster)
{
    if (!name || name[0] != '/' || name[1] == '\0') {
        return ERR;
    }

    if (!dot_only_in_last_entry(name)) {
        return ERR;
    }

    int i = 1;
    int j = 0;

    while (name[i] != '\0') {
        if (name[i] == '/') {
            fat_directory_entry_t *entry = fat_get_dir_entry(*cluster, dir_name);
            if (!entry) {
                return ERR;
            }

            if (entry->attr != ATTR_DIRECTORY) {
                kfree(entry);
                return ERR;
            }

            *cluster = entry->first_cluster_low;
            kfree(entry);

            mem_set(dir_name, ' ', 11);
            j = 0;
            i++;
        } else {
            dir_name[j++] = name[i++];
            if (j > 10) {
                return ERR;
            }
        }
    }

    rearrange_name(dir_name);

    fat_directory_entry_t *existing_entry = fat_get_dir_entry(*cluster, dir_name);
    return existing_entry;
}