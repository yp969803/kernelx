#include "../cpu/boot.h"
#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../drivers/timer.h"
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
    sleep(4000);
    kprintf("Task 1 woke up after sleeping for 2000 ms\n");
    exit();
    return NULL;
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
    initialize_multitasking();
    initialize_timer();
    create_task(task1, NULL, INIT_PAGE_DIR_PHY);

    while (1) {
        halt();
    }
}