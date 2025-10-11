#include "../cpu/idt.h"
#include "../drivers/vga.h"

void main(uint32_t magic, uint32_t mb_addr) {
    if (magic != 0x2BADB002 || mb_addr == 0) {
        vga_print_string("Invalid magic number from bootloader!\n");
        return;
    }

    enable_cursor(14, 15); 
    clear_screen();
    
    idt_init();
    
    while(1) {
        __asm__ __volatile__("hlt"); 
    }
}