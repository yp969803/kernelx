#include "shell.h"
#include "../drivers/keyboard.h"
#include "../drivers/vga.h"
#include "../stdlib/stdio.h"
#include "../stdlib/string.h"
#include "task.h"
#include <stddef.h>

static void shell_execute(const char *line)
{
    if (streq(line, "", 0)) {
        return;
    }

    if (streq(line, "help", 0)) {
        kprintf("help clear\n");
        return;
    }

    if (streq(line, "clear", 0)) {
        clear_screen();
        return;
    }

    kprintf("echo: %s\n", line);
}

void *shell_task(void *args)
{
    (void)args;

    char line[KEYBOARD_LINE_MAX];

    while (1) {
        kprintf("kernelx> ");
        if (keyboard_read_line(line, sizeof(line)) >= 0) {
            shell_execute(line);
        }
    }

    return NULL;
}
