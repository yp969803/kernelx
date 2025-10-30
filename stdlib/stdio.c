#include "stdio.h"


void kprintf(const char *format, ...) 
{
    va_list args;
    va_start(args, format);
    for (const char *p = format; *p; p++){
        if (*p != '%') {
            vga_put_char(*p);
            continue;
        }
        p++;

        switch (*p) {
            case 'c': {
                char c = (char)va_arg(args, int);
                vga_put_char(c);
                break;
            }
            case 's': {
                const char *s = va_arg(args, const char *);
                vga_print_string(s);
                break;
            }
            case 'd': {
                int d = va_arg(args, int);
                vga_print_int(d);
                break;
            }
            case 'x': {
                unsigned int x = va_arg(args, unsigned int);
                vga_print_hex(x);
                break;
            }
            default:
                vga_put_char('%');
                vga_put_char(*p);
        }
    }
}