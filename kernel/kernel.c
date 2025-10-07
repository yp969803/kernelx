#include "../cpu/idt.h"
#include "../drivers/vga.h"

void main() {
    clear_screen();
    
    idt_init();
    
    while(1) {
        __asm__ __volatile__("hlt"); 
    }
}