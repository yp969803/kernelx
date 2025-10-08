#include <stdint.h>
#include "../cpu/io.h"
#include "vga.h"

#define KEYBOARD_DATA_PORT 0x60

void keyboard_handler_c() {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    vga_put_char(scancode & 0x0F);
}
