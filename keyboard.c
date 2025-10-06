#include <stdint.h>

#define KEYBOARD_DATA_PORT 0x60

static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    asm volatile ("inb %1, %0" : "=a"(result) : "dN"(port));
    return result;
}

void keyboard_handler_c() {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    // Example: write scancode to VGA memory at top left
    volatile char* video = (char*)0xB8000;
    video[0] = 'S';                // show a character
    video[1] = 0x07;               // white on black
    video[2] = 'C';
    video[3] = 0x07;
    video[4] = ' ';
    video[5] = 0x07;
    video[6] = '0' + (scancode & 0x0F); // show last hex digit of scancode
    video[7] = 0x07;
}
