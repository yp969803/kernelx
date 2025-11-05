#include "ata.h"
#include "../cpu/io.h"
#include "../kernel/utils.h"

ATA_Device disk;

static inline void tiny_delay()
{
    asm volatile("jmp 1f\n1: jmp 2f\n2:"); // jmp $+2 equivalent
}

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
    if (!dev || !buffer || sector_count == 0) {
        return -1;
    }

    if (lba + sector_count > dev->size_in_sectors) {
        return -1;
    }

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

    if (!(inb(io_base + 7) & ATA_SR_DRDY)) {
        return -1;
    }

    for (uint8_t i = 0; i < sector_count; i++) {
        if (!ata_wait_drq(io_base)) {
            ata_software_reset(dev);
            set_interrupt();
            return -1;
        }

        for (int j = 0; j < 256; j++) {
            uint16_t data               = inw(io_base);
            buffer[i * 512 + j * 2]     = (uint8_t)(data & 0xFF);
            buffer[i * 512 + j * 2 + 1] = (uint8_t)(data >> 8);
        }

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

int ata_write_sectors(ATA_Device *dev, uint32_t lba, uint8_t sector_count, const uint8_t *buffer)
{
    if (!dev || !buffer || sector_count == 0) {
        return -1;
    }

    if (lba + sector_count > dev->size_in_sectors) {
        return -1;
    }

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
    outb(io_base + 7, ATA_CMD_WRITE_PIO);

    io_delay400ns(ctrl_base);

    if (!(inb(io_base + 7) & ATA_SR_DRDY)) {
        return -1;
    }

    for (uint8_t i = 0; i < sector_count; i++) {
        if (!ata_wait_drq(io_base)) {
            ata_software_reset(dev);
            set_interrupt();
            return -1;
        }

        // Write 256 words (512 bytes)
        for (int j = 0; j < 256; j++) {
            uint16_t data = ((uint16_t)buffer[i * 512 + j * 2 + 1] << 8) | buffer[i * 512 + j * 2];
            outw(io_base, data);
            tiny_delay();
        }

        uint8_t status = inb(io_base + 7);
        if (status & (ATA_SR_ERR | ATA_SR_DF)) {
            ata_software_reset(dev);
            set_interrupt();
            return -1;
        }
    }

    // Flush Cache
    outb(io_base + 7, ATA_CMD_CACHE_FLUSH);
    ata_wait_busy(ctrl_base);

    set_interrupt();
    return 0;
}

void ata_software_reset(ATA_Device *dev)
{
    uint16_t ctrl_base = dev->ctrl_base;
    uint16_t io_base   = dev->io_base;

    outb(ctrl_base, 0x04); // Set SRST bit

    io_delay400ns(ctrl_base);

    outb(ctrl_base, 0x00); // Clear SRST bit

    ata_wait_busy(ctrl_base);
}

void set_initial_disk_info(void)
{
    outb(ATA_PRIMARY_IO + 6, 0xA0);
    outb(ATA_PRIMARY_IO + 2, 0);
    outb(ATA_PRIMARY_IO + 3, 0);
    outb(ATA_PRIMARY_IO + 4, 0);
    outb(ATA_PRIMARY_IO + 5, 0);
    outb(ATA_PRIMARY_IO + 7, ATA_CMD_IDENTIFY);
    io_delay400ns(ATA_PRIMARY_CTRL);
    ata_wait_busy(ATA_PRIMARY_CTRL);
    ata_wait_drq(ATA_PRIMARY_IO);

    uint16_t data = 0;
    for (int j = 0; j < 60; j++) {
        data = inw(ATA_PRIMARY_IO);
    }

    uint32_t total_sectors = 0;
    data                   = inw(ATA_PRIMARY_IO);
    total_sectors |= data;
    data = inw(ATA_PRIMARY_IO);
    total_sectors |= (data << 16);

    disk.size_in_sectors = total_sectors - 1;
}

void init_disk(void)
{
    set_initial_disk_info();
    disk.io_base         = ATA_PRIMARY_IO;
    disk.ctrl_base       = ATA_PRIMARY_CTRL;
    disk.slave           = 0; // Master
    disk.partition_start = 1;
    set_initial_disk_info();
    ata_software_reset(&disk);
}