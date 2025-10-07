#include "vga.h"

void clear_screen() {
    volatile uint16_t* video = (uint16_t*)VGA_ADDRESS;
    uint16_t color = (Black << 4) | White; // Black background, White text
    uint16_t blank = (color << 8) | ' ';
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) { 
        video[i] = blank;
    }
}

void set_cursor(uint8_t x, uint8_t y) {
    uint16_t pos = y * VGA_WIDTH + x;

    outb(INDEX_PORT, 0x0f);             // Select low byte
    outb(DATA_PORT, (uint8_t)(pos & 0xff));

    outb(INDEX_PORT, 0x0e);             // Select high byte
    outb(DATA_PORT, (uint8_t)((pos >> 8) & 0xff));
}

uint16_t get_cursor_position()
{
    uint16_t pos = 0;
    outb(INDEX_PORT, 0x0f);
    pos |= inb(DATA_PORT);
    outb(INDEX_PORT, 0x0e);
    pos |= ((uint16_t)inb(DATA_PORT)) << 8;
    return pos;
}

void disable_cursor()
{
	outb(INDEX_PORT, 0x0a);
	outb(DATA_PORT, 0x20);
}

void enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
	outb(INDEX_PORT, 0x0a);
	outb(DATA_PORT, (inb(DATA_PORT) & 0xc0) | cursor_start);

	outb(INDEX_PORT, 0x0b);
	outb(DATA_PORT, (inb(DATA_PORT) & 0xe0) | cursor_end);
}

