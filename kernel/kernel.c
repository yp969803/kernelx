#include "../cpu/idt.h"
#include "../drivers/vga.h"
#include "../cpu/boot.h"
#include "../cpu/gdt.h"

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

void main(uint32_t magic, struct multiboot_header* mb_addr) {
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
    
    while(1) {
        __asm__ __volatile__("hlt"); 
    }
}