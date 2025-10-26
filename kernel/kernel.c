#include "../cpu/idt.h"
#include "../cpu/boot.h"
#include "../cpu/gdt.h"
#include "../stdlib/stdio.h"
#include "kmalloc.h"
#include "mem.h"
#include "utils.h"
#include "task.h"

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

extern uint32_t _kernel_end;

void* task1(void* arg) {
    kprintf("Task1 is running \n");
    schedule();
    return NULL;
}

void main(uint32_t magic, struct multiboot_info* mb_addr) {
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC ) {
        kprintf("Invalid magic number from bootloader!\n");
        return;
    }
    if(!(mb_addr->flags & (1 << 6))){
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

    init_memory(mb_addr->mem_upper*1024, physicalAllocStart);
    kmallocInit(0x1000);
    initialize_multitasking();
    create_task(task1, (uint32_t)initial_page_dir-KERNEL_START);

    schedule();

    kprintf("Reached end of main kernel thread\n");
    while(1) {
        halt();
    }
}