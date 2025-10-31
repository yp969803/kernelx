#include "timer.h"
#include "../cpu/io.h"
#include "../kernel/task.h"
#include "../stdlib/stdio.h"

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND 0x43
#define PIT_FREQ 1193182

static uint32_t time_elapsed_boot = 0;

void pit_set_timer(uint32_t freq)
{
    if (freq == 0)
        return;
    uint16_t count = (uint16_t)(PIT_FREQ / freq);
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, count & 0xFF);
    outb(PIT_CHANNEL0, (count >> 8) & 0xFF);
}

void pit_set_delay(uint32_t freq)
{
    if (freq == 0)
        return;
    uint16_t count = (uint16_t)(PIT_FREQ / freq);
    outb(PIT_COMMAND, 0x38);
    outb(PIT_CHANNEL0, count & 0xFF);
    outb(PIT_CHANNEL0, (count >> 8) & 0xFF);
}

void initialize_timer(void)
{
    pit_set_timer(1000);
    time_elapsed_boot = 0;
}

void pit_handler_c(void)
{
    time_elapsed_boot++;
    quantum_expired_handler();
}