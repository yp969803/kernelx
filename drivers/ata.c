#include "ata.h"
#include "../cpu/io.h"
#include "../kernel/utils.h"

int ata_read_sectors(ATA_Device* dev, uint32_t lba, uint8_t sector_count, uint8_t* buffer) {
    if(lba < dev->partition_start) {
        return -1; // Out of bounds
    }
    
    if(lba + sector_count > dev->size_in_sectors) {
        return -1; // Out of bounds
    }

    lba += dev->partition_start;
    uint16_t io_base = dev->io_base;
    uint16_t ctrl_base = dev->ctrl_base;
    uint8_t drive_head = 0xE0 | (dev->slave << 4) | ((lba >> 24) & 0x0F);

    // Wait until the drive is not busy
    while (inb(ctrl_base) & 0x80);

    clear_interrupt();

    outb(io_base + 6, drive_head);
    outb(io_base + 2, sector_count);
    outb(io_base + 3, (uint8_t)(lba & 0xFF));
    outb(io_base + 4, (uint8_t)((lba >> 8) & 0xFF));
    outb(io_base + 5, (uint8_t)((lba >> 16) & 0xFF));
    outb(io_base + 7, PIO28_READ_CMD); 

    // delay for 400ns
    for (int i = 0; i < 4; i++) {
        inb(ctrl_base);
    }

    for (uint8_t i = 0; i < sector_count; i++) {
        // Wait for the drive to signal that data is ready
        while (!(inb(io_base+7) & 0x08));

        for (int j = 0; j < 256; j++) {
            uint16_t data = inw(io_base);
            buffer[i * 512 + j * 2] = (uint8_t)(data & 0xFF);
            buffer[i * 512 + j * 2 + 1] = (uint8_t)((data >> 8) & 0xFF);
        }
    }

    set_interrupt();
    return 0;
}

int ata_software_reset(ATA_Device* dev) {
    uint16_t ctrl_base = dev->ctrl_base;
    uint16_t io_base = dev->io_base;

    outb(ctrl_base, 0x04); // Set SRST bit

    // delay for 400ns
    for (int i = 0; i < 4; i++) {
        inb(ctrl_base);
    }

    outb(ctrl_base, 0x00); // Clear SRST bit

    // Wait until the drive is not busy.
    while (inb(ctrl_base) & 0x80);

    // Wait until the drive is ready.
    while (!(inb(io_base + 7) & 0x40));
    
    return 0;
}