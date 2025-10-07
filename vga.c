#include "vga.h"

void clear_screen() {
    volatile uint16_t* video = (uint16_t*)VGA_ADDRESS;
    uint16_t color = (Black << 4) | White; // Black background, White text
    uint16_t blank = (color << 8) | ' ';
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) { 
        video[i] = blank;
    }
}