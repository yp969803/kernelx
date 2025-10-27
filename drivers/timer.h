#pragma once

#include <stdint.h>

extern uint32_t time_elapsed_boot;

void pit_set_timer(uint32_t milliseconds);
void pit_set_delay(uint32_t milliseconds);
void initialize_timer(void);