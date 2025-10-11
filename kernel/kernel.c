#include "../cpu/idt.h"
#include "../drivers/vga.h"

void main() {
    enable_cursor(14, 15); 
    clear_screen();
    
    idt_init();
    
    while(1) {
        __asm__ __volatile__("hlt"); 
    }
}