#include "idt.h"

void main() {
    char* video_memory = (char*) 0xb8000;
    *video_memory = 'Y';
    idt_init();
    while(1) {
        __asm__ __volatile__("hlt"); 
    }
}