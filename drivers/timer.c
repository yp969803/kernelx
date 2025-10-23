#include "timer.h"
#include "../cpu/io.h"


#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_FREQ     1193182


void pit_set_timer(uint32_t milliseconds) {
    uint16_t count = (uint16_t)(PIT_FREQ * milliseconds / 1000);
    outb(PIT_COMMAND, 0x36); 
    outb(PIT_CHANNEL0, count & 0xFF); 
    outb(PIT_CHANNEL0, (count >> 8) & 0xFF); 
}

void pit_set_delay(uint32_t milliseconds) {
    uint16_t count = (uint16_t)(PIT_FREQ * milliseconds / 1000);
    outb(PIT_COMMAND, 0x38);
    outb(PIT_CHANNEL0, count & 0xFF);
    outb(PIT_CHANNEL0, (count >> 8) & 0xFF);
}

void pit_handler_c(void){

}