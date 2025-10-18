#include "../cpu/idt.h"
#include "../drivers/vga.h"
#include "../cpu/boot.h"
#include "../cpu/gdt.h"
#include "mem.h"

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

void main(uint32_t magic, struct multiboot_info* mb_addr) {
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC ) {
        vga_print_string("Invalid magic number from bootloader!\n");
        return;
    }
    if(!(mb_addr->flags & (1 << 6))){
        vga_print_string("Memory Info not provided by bootloader!\n");
        return;
    }
    initGdt();

    enable_cursor(14, 15); 
    clear_screen();
    
    idt_init();

    uint32_t mod1 = *(uint32_t*)(mb_addr->mods_addr+4);
    uint32_t physicalAllocStart = (mod1 + 0xFFF) & ~0xFFF;
    
    init_memory(mb_addr->mem_upper*1024, physicalAllocStart);
    
    while(1) {
        __asm__ __volatile__("hlt"); 
    }
}