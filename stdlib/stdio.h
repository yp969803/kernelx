#pragma once

#include "../drivers/vga.h"
#include <stdarg.h>
#include <stdint.h>

void kprintf(const char *format, ...);