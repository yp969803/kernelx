#include "idt.h"

void main() {
    idt_init();
    while(1) {
        __asm__ __volatile__("hlt"); 
    }
}