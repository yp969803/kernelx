#include "../cpu/io.h"
#include "../kernel/utils.h"
#include "keyboard.h"
#include "vga.h"
#include <stdbool.h>
#include <stdint.h>

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

static const char keymap[60] = {0,   0,   '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
                                '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
                                'o', 'p', '[',  ']',  '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
                                'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
                                'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' ', 0};

static const char shift_keymap[60] = {0,   27,  '!',  '@',  '#',  '$', '%', '^', '&', '*', '(', ')',
                                      '_', '+', '\b', '\t', 'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',
                                      'O', 'P', '{',  '}',  '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H',
                                      'J', 'K', 'L',  ':',  '"',  '~', 0,   '|', 'Z', 'X', 'C', 'V',
                                      'B', 'N', 'M',  '<',  '>',  '?', 0,   '*', 0,   ' ', 0};

static bool shift_pressed = false;
static char key_buffer[KEYBOARD_BUFFER_SIZE];
static uint32_t key_head = 0;
static uint32_t key_tail = 0;

static bool keybuf_empty(void)
{
    return key_head == key_tail;
}

static bool keybuf_full(void)
{
    return ((key_head + 1) % KEYBOARD_BUFFER_SIZE) == key_tail;
}

static void keybuf_push(char ch)
{
    if (keybuf_full()) {
        return;
    }

    key_buffer[key_head] = ch;
    key_head             = (key_head + 1) % KEYBOARD_BUFFER_SIZE;
}

static int keybuf_pop(void)
{
    if (keybuf_empty()) {
        return ERR;
    }

    char ch  = key_buffer[key_tail];
    key_tail = (key_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return ch;
}

void keyboard_init(void)
{
    unsigned long flags = save_irqdisable();
    key_head            = 0;
    key_tail            = 0;
    shift_pressed       = false;
    irqrestore(flags);
}

int keyboard_getchar(void)
{
    while (1) {
        unsigned long flags = save_irqdisable();
        int ch              = keybuf_pop();
        irqrestore(flags);

        if (ch >= 0) {
            return ch;
        }

        halt();
    }
}

int keyboard_read_line(char *buffer, uint32_t max)
{
    if (!buffer || max == 0) {
        return ERR;
    }

    uint32_t len = 0;

    while (1) {
        char ch = (char)keyboard_getchar();

        if (ch == '\n') {
            buffer[len] = '\0';
            vga_put_char('\n');
            return (int)len;
        }

        if (ch == '\b') {
            if (len > 0) {
                len--;
                buffer[len] = '\0';
                vga_remove_char();
            }
            continue;
        }

        if (len < max - 1) {
            buffer[len++] = ch;
            vga_put_char((uint8_t)ch);
        }
    }
}

void keyboard_handler_c(void)
{
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    if (scancode == LEFT_SHIFT || scancode == RIGHT_SHIFT) {
        shift_pressed = true;
        return;
    }

    if (scancode == (LEFT_SHIFT | 0x80) || scancode == (RIGHT_SHIFT | 0x80)) {
        shift_pressed = false;
        return;
    }

    if (scancode & 0x80 || scancode >= 60 || keymap[scancode] == 0 || scancode == 0xe0) {
        return;
    }

    char ch = shift_pressed ? shift_keymap[scancode] : keymap[scancode];
    keybuf_push(ch);
}
