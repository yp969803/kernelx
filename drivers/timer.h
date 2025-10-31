#pragma once

#include "../kernel/task.h"
#include <stdint.h>

typedef struct sleep_info {
    uint32_t wake_time; // in ms
    thread_control_block *task;
    struct sleep_info *next;
} sleep_info;

void pit_set_timer(uint32_t milliseconds);
void pit_set_delay(uint32_t milliseconds);
void initialize_timer(void);
void sleep(uint32_t milliseconds);