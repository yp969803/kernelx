#include "../cpu/boot.h"
#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../drivers/ata.h"
#include "../drivers/timer.h"
#include "../fs/fat.h"
#include "../stdlib/stdio.h"
#include "kmalloc.h"
#include "mem.h"
#include "task.h"
#include "utils.h"

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

extern uint32_t _kernel_end;

void *task1(void *args)
{
    kprintf("Task 1 is running\n");
    sleep(1000);
    kprintf("Task 1 woke up after sleeping for 1s\n");
    exit();
    return NULL;
}

void *task2(void *args)
{
    for (int i = 1; i <= 3; i++) {
        kprintf("Task 2 iteration %d\n", i);
        sleep(500);
    }

    kprintf("Task 2 finished\n");
    exit();
    return NULL;
}

static bool buffer_equals(const uint8_t *a, const uint8_t *b, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

static int fat_write_read_verify(const char *path, const uint8_t *data, uint32_t data_size)
{
    fat_rm_entry(path);

    if (mkfile_fat(path) != OK) {
        return ERR;
    }

    char dir_name[11];
    mem_set(dir_name, ' ', 11);
    uint16_t parent_cluster = ROOT_CLUSTER;
    fat_directory_entry_t *entry = fat_lookup(path, dir_name, &parent_cluster);
    if (!entry) {
        return ERR;
    }

    uint16_t file_cluster = entry->first_cluster_low;
    if (fat_write_file(&file_cluster, data, data_size) != OK) {
        kfree(entry);
        return ERR;
    }

    entry->first_cluster_low = file_cluster;
    entry->file_size         = data_size;
    if (fat_update_dir_entry(parent_cluster, dir_name, entry) != OK) {
        kfree(entry);
        return ERR;
    }

    uint8_t *read_buffer = kmalloc(data_size);
    if (!read_buffer) {
        kfree(entry);
        return ERR;
    }

    mem_set(read_buffer, 0, data_size);
    uint16_t read_cluster = entry->first_cluster_low;
    if (fat_read_file(&read_cluster, read_buffer, data_size) != OK) {
        kfree(read_buffer);
        kfree(entry);
        return ERR;
    }

    kfree(entry);

    if (!buffer_equals(data, read_buffer, data_size)) {
        kfree(read_buffer);
        return ERR;
    }
    kfree(read_buffer);

    if (fat_rm_entry(path) != OK) {
        return ERR;
    }

    return OK;
}

static void fill_pattern(uint8_t *buffer, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = (uint8_t)((i * 31 + 7) & 0xFF);
    }
}

static void filesystem_smoke_test(void)
{
    const uint8_t small_data[] = "kernelx-fat-smoke";

    kprintf("FAT root file: ");
    if (fat_write_read_verify("/fstest.txt", small_data, sizeof(small_data) - 1) != OK) {
        kprintf("FAIL\n");
        return;
    }
    kprintf("PASS\n");

    kprintf("FAT nested big file: ");
    fat_rm_entry("/fsdir/big.bin");
    fat_rm_entry("/fsdir");

    if (mkdir_fat("/fsdir") != OK) {
        kprintf("FAIL mkdir\n");
        return;
    }

    const uint32_t big_size = 9000;
    uint8_t *big_data       = kmalloc(big_size);
    if (!big_data) {
        kprintf("FAIL alloc\n");
        return;
    }

    fill_pattern(big_data, big_size);
    if (fat_write_read_verify("/fsdir/big.bin", big_data, big_size) != OK) {
        kfree(big_data);
        kprintf("FAIL file\n");
        return;
    }
    kfree(big_data);

    if (fat_rm_entry("/fsdir") != OK) {
        kprintf("FAIL cleanup\n");
        return;
    }

    kprintf("PASS\n");
}

void main(uint32_t magic, struct multiboot_info *mb_addr)
{
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        kprintf("Invalid magic number from bootloader!\n");
        return;
    }

    if (!(mb_addr->flags & (1 << 6))) {
        kprintf("Memory Info not provided by bootloader!\n");
        return;
    }

    initGdt();

    enable_cursor(14, 15);
    clear_screen();

    idt_init();

    // uint32_t mod1 = *(uint32_t*)(mb_addr->mods_addr+4);
    // uint32_t physicalAllocStart = (mod1 + 0xFFF) & ~0xFFF;
    uint32_t physicalAllocStart = ((uint32_t)&_kernel_end - 0xC0000000 + 0xFFF) & ~0xFFF;

    init_memory(mb_addr->mem_upper * 1024, physicalAllocStart);
    kmallocInit(0x1000);
    init_disk();
    filesystem_smoke_test();
    initialize_multitasking();
    initialize_timer();
    create_kernel_task(task1, NULL);
    create_kernel_task(task2, NULL);

    while (1) {
        halt();
    }
}
