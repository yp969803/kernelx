#include "idt.h"
#include "vga.h"

void main() {
    vga_init();
    
    idt_init();
    
    while(1) {
        __asm__ __volatile__("hlt"); 
    }
}