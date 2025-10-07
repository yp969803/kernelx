#include <stdint.h>
#include "../cpu/io.h"

#define VGA_ADDRESS 0xb8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define VGA_INDEX_PORT 0x3d4
#define VGA_DATA_PORT 0x3d5

#define Black   0x0
#define Blue    0x1
#define Green	0x2
#define Cyan	0x3
#define Red	    0x4
#define Magenta	0x5
#define Brown	0x6
#define Gray	0x7
#define DarkGray	0x8
#define BrightBlue	0x9
#define BrightGreen	0xa
#define BrightCyan	0xb
#define BrightRed	0xc
#define BrightMagenta	0xd
#define Yellow	0xe
#define White	0xf

void clear_screen();
void set_cursor(uint8_t x, uint8_t y);
uint16_t get_cursor_position();
// void disable_cursor();
// void enable_cursor(uint8_t cursor_start, uint8_t cursor_end);
void vga_init();
