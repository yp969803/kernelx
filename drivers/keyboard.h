#pragma once

#include <stdint.h>

#define KEYBOARD_BUFFER_SIZE 256
#define KEYBOARD_LINE_MAX 128

void keyboard_init(void);
int keyboard_getchar(void);
int keyboard_read_line(char *buffer, uint32_t max);

