#include <stdint.h>
#include <stdbool.h>
#include "../cpu/io.h"
#include "vga.h"

#define KEYBOARD_DATA_PORT 0x60

#define EXTENDED_SCANCODE 0xe0

#define ENTER 0x1C
#define BACKSPACE 0x0E
#define ESCAPE 0x01
#define LEFT_SHIFT 0x2A 	
#define RIGHT_SHIFT 0x36
#define CAPS_LOCK 0x3A
#define ESC 0x01

// Key release = scancode + 0x80

static const char keymap[60] = {
    0,  0, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
    'z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',0};

static const char shift_keymap[60] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,
    'A','S','D','F','G','H','J','K','L',':','"','~',0,'|',
    'Z','X','C','V','B','N','M','<','>','?',0,'*',0,' ',0
};

static bool shift_pressed = false;

void keyboard_handler_c(void) 
{
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    if(scancode==LEFT_SHIFT || scancode==RIGHT_SHIFT){
        shift_pressed = true;
        return;
    }

    if(scancode==(LEFT_SHIFT | 0x80) || scancode==(RIGHT_SHIFT | 0x80)){
        shift_pressed = false;
        return;
    }

    if(scancode & 0x80 || scancode >= 60 || keymap[scancode] == 0 ||  scancode == 0xe0) {
        return;
    }

    if(scancode == BACKSPACE) {
        vga_remove_char();
        return;
    }
    char ch= shift_pressed ? shift_keymap[scancode] : keymap[scancode];
    vga_put_char(ch);
}
