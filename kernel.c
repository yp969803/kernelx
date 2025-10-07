#include "idt.h"
#include "vga.h"

void main() {
    clear_screen();
    idt_init();
    
    while(1) {
        __asm__ __volatile__("hlt"); 
    }
}