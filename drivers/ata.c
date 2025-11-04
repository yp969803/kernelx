#include "ata.h"
#include "../cpu/io.h"
#include "../kernel/utils.h"

ATA_Device disk;

static inline void io_delay400ns(uint16_t ctrl_base)
{
    for (int i = 0; i < 4; i++) {
        inb(ctrl_base);
    }
}

static bool ata_wait_busy(uint16_t ctrl_base)
{
    while (inb(ctrl_base) & ATA_SR_BSY)
        ;
    return true;
}

static bool ata_wait_drq(uint16_t io_base)
{
    uint8_t status;
    do {
        status = inb(io_base + 7);
        if (status & ATA_SR_ERR)
            return false;
    } while (!(status & ATA_SR_DRQ));
    return true;
}

int ata_read_sectors(ATA_Device *dev, uint32_t lba, uint8_t sector_count, uint8_t *buffer)
{
    if (!dev || !buffer || sector_count == 0)
        return -1;

    if (lba < dev->partition_start || (lba + sector_count) > dev->size_in_sectors)
        return -1;

    lba += dev->partition_start;
    uint16_t io_base   = dev->io_base;
    uint16_t ctrl_base = dev->ctrl_base;

    uint8_t drive_head = 0xE0 | (dev->slave << 4) | ((lba >> 24) & 0x0F);

    ata_wait_busy(ctrl_base);
    clear_interrupt();

    outb(io_base + 6, drive_head);
    outb(io_base + 2, sector_count);
    outb(io_base + 3, (uint8_t)(lba & 0xFF));
    outb(io_base + 4, (uint8_t)((lba >> 8) & 0xFF));
    outb(io_base + 5, (uint8_t)((lba >> 16) & 0xFF));
    outb(io_base + 7, ATA_CMD_READ_PIO);

    io_delay400ns(ctrl_base);

    for (uint8_t i = 0; i < sector_count; i++) {
        if (!ata_wait_drq(io_base)) {
            ata_software_reset(dev);
            set_interrupt();
            return -1;
        }

        // Read 256 words (512 bytes)
        for (int j = 0; j < 256; j++) {
            uint16_t data               = inw(io_base);
            buffer[i * 512 + j * 2]     = (uint8_t)(data & 0xFF);
            buffer[i * 512 + j * 2 + 1] = (uint8_t)(data >> 8);
        }

        // After each sector, check for error or DF (Device Fault)
        uint8_t status = inb(io_base + 7);
        if (status & (ATA_SR_ERR | ATA_SR_DF)) {
            ata_software_reset(dev);
            set_interrupt();
            return -1;
        }
    }

    set_interrupt();
    return 0;
}

int ata_software_reset(ATA_Device *dev)
{
    uint16_t ctrl_base = dev->ctrl_base;
    uint16_t io_base   = dev->io_base;

    outb(ctrl_base, 0x04); // Set SRST bit

    io_delay400ns(ctrl_base);

    outb(ctrl_base, 0x00); // Clear SRST bit

    ata_wait_busy(ctrl_base);

    // Wait until the drive is ready.
    while (!(inb(io_base + 7) & ATA_SR_DRDY))
        ;

    return 0;
}

void init_disk(void)
{
    disk.io_base         = ATA_PRIMARY_IO;
    disk.ctrl_base       = ATA_PRIMARY_CTRL;
    disk.slave           = 0; // Master
    disk.size_in_sectors = 131070;
    disk.partition_start = 1;
    ata_software_reset(&disk);
}