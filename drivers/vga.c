#include "vga.h"
#include "../kernel/mem.h"
#include <string.h>

// Default color: Black background, White foreground
static uint16_t color = (Black << 4) | White;

void clear_screen(void) {
    volatile uint16_t* video = (uint16_t*)VGA_ADDRESS;
    uint16_t blank = (color << 8) | ' ';
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) { 
        video[i] = blank;
    }
    set_cursor(0);
}

void set_cursor(uint16_t pos) {
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

void disable_cursor(void)
{
	outb(VGA_INDEX_PORT, 0x0a);
	outb(VGA_DATA_PORT, 0x20);
}

void enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
	outb(VGA_INDEX_PORT, 0x0a);
	outb(VGA_DATA_PORT, (inb(VGA_DATA_PORT) & 0xc0) | cursor_start);

	outb(VGA_INDEX_PORT, 0x0b);
	outb(VGA_DATA_PORT, (inb(VGA_DATA_PORT) & 0xe0) | cursor_end);
}

void vga_put_char(uint8_t c) {
    volatile uint16_t* vga = (volatile uint16_t*)VGA_ADDRESS;
    uint16_t pos = get_cursor_position();

    if (c == '\n') {
        pos += VGA_WIDTH - (pos % VGA_WIDTH); // move to next line
    } else {
        vga[pos] = (color << 8) | c;
        pos++;
    }

    if (pos >= VGA_WIDTH * VGA_HEIGHT) {
        memmove(
        (void*)vga,
        (void*)(vga + VGA_WIDTH),
        (VGA_HEIGHT - 1) * VGA_WIDTH * 2
       );

       for (int col = 0; col < VGA_WIDTH; col++) {
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = (color << 8) | ' ';
       }

        pos = (VGA_HEIGHT - 1) * VGA_WIDTH; 
    }
    set_cursor(pos);
}

void vga_print_string(const char* str) {
    int i = 0;
    while (str[i] != '\0') {
        vga_put_char(str[i]);
        i++;
    }   
}

void vga_remove_char(void){
    uint16_t pos = get_cursor_position();
    if(pos == 0) return;
    pos--;
    volatile uint16_t* vga = (volatile uint16_t*)VGA_ADDRESS;
    vga[pos] = (color << 8) | ' ';
    set_cursor(pos);
}

static void get_digits(char *buf, int num, int *i) {
  if (num == 0) {
    buf[(*i)++] = '0';
    return;
  }

  int is_negative = 0;
  if (num < 0) {
    is_negative = 1;
    num = -num;
  }

  while (num > 0) {
    buf[(*i)++] = '0' + (num % 10);
    num /= 10;
  }

  if (is_negative) {
    buf[(*i)++] = '-';
  }
}

void vga_print_int(int num) {
  char buf[12];
  int i = 0;
  get_digits(buf, num, &i);

  while (i--) {
    vga_put_char(buf[i]);
  }
}

void vga_print_hex(uint32_t num) {
  char hex_chars[] = "0123456789ABCDEF";

  vga_print_string("0x");

  for (int i = 28; i >= 0; i -= 4) {
    char c = hex_chars[(num >> i) & 0xF];
    vga_put_char(c);
  }
}
