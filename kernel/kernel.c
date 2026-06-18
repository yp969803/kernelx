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

static void filesystem_smoke_test(void)
{
    const char path[] = "/fstest.txt";
    const uint8_t data[] = "kernelx-fat-smoke";
    const uint32_t data_size = sizeof(data) - 1;
    uint8_t read_buffer[sizeof(data)];

    kprintf("FAT smoke: ");

    fat_rm_entry(path);

    if (mkfile_fat(path) != OK) {
        kprintf("FAIL mkfile\n");
        return;
    }

    char dir_name[11];
    mem_set(dir_name, ' ', 11);
    uint16_t parent_cluster = ROOT_CLUSTER;
    fat_directory_entry_t *entry = fat_lookup(path, dir_name, &parent_cluster);
    if (!entry) {
        kprintf("FAIL lookup\n");
        return;
    }

    uint16_t file_cluster = entry->first_cluster_low;
    if (fat_write_file(&file_cluster, data, data_size) != OK) {
        kfree(entry);
        kprintf("FAIL write\n");
        return;
    }

    entry->first_cluster_low = file_cluster;
    entry->file_size         = data_size;
    if (fat_update_dir_entry(parent_cluster, dir_name, entry) != OK) {
        kfree(entry);
        kprintf("FAIL update\n");
        return;
    }

    mem_set(read_buffer, 0, sizeof(read_buffer));
    uint16_t read_cluster = entry->first_cluster_low;
    if (fat_read_file(&read_cluster, read_buffer, data_size) != OK) {
        kfree(entry);
        kprintf("FAIL read\n");
        return;
    }

    kfree(entry);

    if (!buffer_equals(data, read_buffer, data_size)) {
        kprintf("FAIL compare\n");
        return;
    }

    if (fat_rm_entry(path) != OK) {
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
