#include <stdint.h>
#include "../cpu/io.h"
#include "vga.h"

#define KEYBOARD_DATA_PORT 0x60

#define EXTENDED_SCANCODE 0xe0

#define ENTER 0x1C
#define BACKSPACE 0x0E
#define ESCAPE 0x01
#define LEFT_SHIFT 0x2A 	
#define CAPS_LOCK 0x3A

// Key release = scancode + 0x80

const char keymap[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
    'z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',0};

void keyboard_handler_c() {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    if(scancode & 0x80 || scancode >= 128 || keymap[scancode] == 0 || keymap[scancode]==27 || scancode == 0xe0) {
        return;
    }

    if(scancode == BACKSPACE) {
        vga_remove_char();
        return;
    }
    vga_put_char(keymap[scancode]);
}
