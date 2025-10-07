#include "vga.h"

void clear_screen(void) {
    volatile uint16_t* video = (uint16_t*)VGA_ADDRESS;
    uint16_t color = (Black << 4) | White; // Black background, White text
    uint16_t blank = (color << 8) | ' ';
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) { 
        video[i] = blank;
    }
    set_cursor(0, 0);
}

void set_cursor(uint8_t x, uint8_t y) {
    uint16_t pos = y * VGA_WIDTH + x;

    outb(VGA_INDEX_PORT, 0x0f);             // Select low byte
    outb(VGA_DATA_PORT, (uint8_t)(pos & 0xff));

    outb(VGA_INDEX_PORT, 0x0e);             // Select high byte
    outb(VGA_DATA_PORT, (uint8_t)((pos >> 8) & 0xff));
}

uint16_t get_cursor_position(void)
{
    uint16_t pos = 0;
    outb(VGA_INDEX_PORT, 0x0f);
    pos |= inb(VGA_DATA_PORT);
    outb(VGA_INDEX_PORT, 0x0e);
    pos |= ((uint16_t)inb(VGA_DATA_PORT)) << 8;
    return pos;
}

// void disable_cursor()
// {
// 	outb(VGA_INDEX_PORT, 0x0a);
// 	outb(VGA_DATA_PORT, 0x20);
// }

// void enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
// {
// 	outb(VGA_INDEX_PORT, 0x0a);
// 	outb(VGA_DATA_PORT, (inb(VGA_DATA_PORT) & 0xc0) | cursor_start);

// 	outb(VGA_INDEX_PORT, 0x0b);
// 	outb(VGA_DATA_PORT, (inb(VGA_DATA_PORT) & 0xe0) | cursor_end);
// }

