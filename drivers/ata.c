#include "ata.h"
#include "../cpu/io.h"
#include "../fs/fat.h"
#include "../kernel/mem.h"
#include "../kernel/task.h"
#include "../kernel/utils.h"

ATA_Device disk;

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC
#define PCI_COMMAND_BUS_MASTER 0x0004
#define PCI_IDE_CLASS 0x01
#define PCI_IDE_SUBCLASS 0x01

#define BMIDE_PRIMARY_COMMAND 0
#define BMIDE_PRIMARY_STATUS 2
#define BMIDE_PRIMARY_PRDT 4
#define BMIDE_CMD_START 0x01
#define BMIDE_CMD_READ 0x08
#define BMIDE_STATUS_ERROR 0x02
#define BMIDE_STATUS_INTERRUPT 0x04

#define ATA_DMA_CHUNK_SECTORS 8
#define ATA_DMA_TIMEOUT 10000000

typedef struct {
    uint32_t base;
    uint16_t byte_count;
    uint16_t flags;
} __attribute__((packed)) ATA_PRD;

static uint8_t dma_bounce_buffer[ATA_DMA_CHUNK_SECTORS * ATA_SECTOR_SIZE]
    __attribute__((aligned(4096)));
static ATA_PRD dma_prd __attribute__((aligned(16)));

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

static uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t address = (uint32_t)(0x80000000 | ((uint32_t)bus << 16) | ((uint32_t)slot << 11) |
                                  ((uint32_t)func << 8) | (offset & 0xFC));
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

static void pci_config_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset,
                             uint32_t value)
{
    uint32_t address = (uint32_t)(0x80000000 | ((uint32_t)bus << 16) | ((uint32_t)slot << 11) |
                                  ((uint32_t)func << 8) | (offset & 0xFC));
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

static int ata_pci_init_dma(void)
{
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t id = pci_config_read((uint8_t)bus, slot, func, 0x00);
                if (id == 0xFFFFFFFF) {
                    if (func == 0) {
                        break;
                    }
                    continue;
                }

                uint32_t class_reg = pci_config_read((uint8_t)bus, slot, func, 0x08);
                uint8_t class_code = (uint8_t)(class_reg >> 24);
                uint8_t subclass   = (uint8_t)(class_reg >> 16);
                if (class_code != PCI_IDE_CLASS || subclass != PCI_IDE_SUBCLASS) {
                    continue;
                }

                uint32_t command = pci_config_read((uint8_t)bus, slot, func, 0x04);
                command |= PCI_COMMAND_BUS_MASTER;
                pci_config_write((uint8_t)bus, slot, func, 0x04, command);

                uint32_t bar4 = pci_config_read((uint8_t)bus, slot, func, 0x20);
                uint16_t base = (uint16_t)(bar4 & ~0x0F);
                if (base == 0) {
                    return ERR;
                }

                disk.bmide_base = base;
                disk.dma_ready  = 1;
                return OK;
            }
        }
    }

    disk.dma_ready = 0;
    return ERR;
}

static uint32_t kernel_virt_to_phys(void *ptr)
{
    return (uint32_t)ptr - KERNEL_START;
}

static void ata_copy(uint8_t *dst, const uint8_t *src, uint32_t bytes)
{
    for (uint32_t i = 0; i < bytes; i++) {
        dst[i] = src[i];
    }
}

static int ata_wait_dma_irq(unsigned long saved_flags)
{
    set_interrupt();

    for (uint32_t timeout = 0; timeout < ATA_DMA_TIMEOUT; timeout++) {
        if (disk.irq_seen) {
            irqrestore(saved_flags);
            return OK;
        }

        if (inb(disk.bmide_base + BMIDE_PRIMARY_STATUS) & BMIDE_STATUS_ERROR) {
            disk.dma_error = 1;
            break;
        }

        halt();
    }

    irqrestore(saved_flags);
    return ERR;
}

static int ata_dma_transfer_chunk(uint32_t lba, uint8_t sector_count, uint8_t *buffer,
                                  bool is_write)
{
    if (!disk.dma_ready || sector_count == 0 || sector_count > ATA_DMA_CHUNK_SECTORS) {
        return ERR;
    }

    uint32_t bytes = (uint32_t)sector_count * ATA_SECTOR_SIZE;
    if (is_write) {
        ata_copy(dma_bounce_buffer, buffer, bytes);
    }

    uint16_t io_base   = disk.io_base;
    uint16_t ctrl_base = disk.ctrl_base;
    uint8_t drive_head = 0xE0 | (disk.slave << 4) | ((lba >> 24) & 0x0F);

    ata_wait_busy(ctrl_base);
    unsigned long flags = save_irqdisable();

    disk.irq_seen   = 0;
    disk.dma_error  = 0;
    disk.dma_active = 1;

    dma_prd.base       = kernel_virt_to_phys(dma_bounce_buffer);
    dma_prd.byte_count = (uint16_t)bytes;
    dma_prd.flags      = 0x8000;

    outb(disk.bmide_base + BMIDE_PRIMARY_COMMAND, 0);
    outl(disk.bmide_base + BMIDE_PRIMARY_PRDT, kernel_virt_to_phys(&dma_prd));
    outb(disk.bmide_base + BMIDE_PRIMARY_STATUS, BMIDE_STATUS_INTERRUPT | BMIDE_STATUS_ERROR);
    outb(disk.bmide_base + BMIDE_PRIMARY_COMMAND, is_write ? 0 : BMIDE_CMD_READ);

    outb(io_base + 6, drive_head);
    outb(io_base + 2, sector_count);
    outb(io_base + 3, (uint8_t)(lba & 0xFF));
    outb(io_base + 4, (uint8_t)((lba >> 8) & 0xFF));
    outb(io_base + 5, (uint8_t)((lba >> 16) & 0xFF));
    outb(io_base + 7, is_write ? ATA_CMD_WRITE_DMA : ATA_CMD_READ_DMA);

    io_delay400ns(ctrl_base);

    if (!(inb(io_base + 7) & ATA_SR_DRDY)) {
        disk.dma_active = 0;
        irqrestore(flags);
        return ERR;
    }

    outb(disk.bmide_base + BMIDE_PRIMARY_COMMAND,
         BMIDE_CMD_START | (is_write ? 0 : BMIDE_CMD_READ));

    int wait_result = ata_wait_dma_irq(flags);

    unsigned long stop_flags = save_irqdisable();
    outb(disk.bmide_base + BMIDE_PRIMARY_COMMAND, is_write ? 0 : BMIDE_CMD_READ);
    uint8_t bm_status = inb(disk.bmide_base + BMIDE_PRIMARY_STATUS);
    outb(disk.bmide_base + BMIDE_PRIMARY_STATUS,
         bm_status | BMIDE_STATUS_INTERRUPT | BMIDE_STATUS_ERROR);
    uint8_t ata_status = inb(io_base + 7);
    disk.dma_active    = 0;
    irqrestore(stop_flags);

    if (wait_result != OK || disk.dma_error || (bm_status & BMIDE_STATUS_ERROR) ||
        (ata_status & (ATA_SR_ERR | ATA_SR_DF))) {
        ata_software_reset();
        return ERR;
    }

    if (!is_write) {
        ata_copy(buffer, dma_bounce_buffer, bytes);
    }

    return OK;
}

static int ata_dma_transfer(uint32_t lba, uint8_t sector_count, uint8_t *buffer, bool is_write)
{
    if (!buffer || sector_count == 0 || !disk.dma_ready) {
        return ERR;
    }

    if (lba > disk.size_in_sectors || sector_count > disk.size_in_sectors - lba) {
        return ERR;
    }

    lba += disk.partition_start;
    task_scheduler_disable();

    uint8_t remaining = sector_count;
    uint32_t offset   = 0;
    while (remaining > 0) {
        uint8_t chunk = remaining > ATA_DMA_CHUNK_SECTORS ? ATA_DMA_CHUNK_SECTORS : remaining;
        if (ata_dma_transfer_chunk(lba, chunk, buffer + offset, is_write) != OK) {
            task_scheduler_enable();
            return ERR;
        }
        lba += chunk;
        offset += (uint32_t)chunk * ATA_SECTOR_SIZE;
        remaining -= chunk;
    }

    task_scheduler_enable();
    return OK;
}

int ata_read_sectors(uint32_t lba, uint8_t sector_count, uint8_t *buffer)
{
    return ata_dma_transfer(lba, sector_count, buffer, false);
}

int ata_write_sectors(uint32_t lba, uint8_t sector_count, const uint8_t *buffer)
{
    int result = ata_dma_transfer(lba, sector_count, (uint8_t *)buffer, true);
    if (result != OK) {
        return ERR;
    }

    unsigned long flags = save_irqdisable();
    outb(disk.io_base + 7, ATA_CMD_CACHE_FLUSH);
    ata_wait_busy(disk.ctrl_base);
    irqrestore(flags);
    return OK;
}

void ata_irq_handler_c(void)
{
    if (!disk.dma_active) {
        return;
    }

    uint8_t bm_status = inb(disk.bmide_base + BMIDE_PRIMARY_STATUS);
    if (bm_status & BMIDE_STATUS_ERROR) {
        disk.dma_error = 1;
    }
    disk.irq_seen = 1;
}

void ata_software_reset(void)
{
    uint16_t ctrl_base = disk.ctrl_base;
    uint16_t io_base   = disk.io_base;

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
    disk.dma_ready       = 0;
    disk.irq_seen        = 0;
    disk.dma_error       = 0;
    disk.dma_active      = 0;
    ata_software_reset();
    if (ata_pci_init_dma() != OK) {
        return;
    }

    if (read_fat_boot_sector() != OK || !fat_boot_sector_valid() || !fat_table_valid()) {
        if (mkfs_fat() != OK) {
            return;
        }
        read_fat_boot_sector();
    }
}
