#pragma once

#include <stdint.h>

typedef struct thread_control_block{
    uint32_t* esp;
    uint32_t* esp0;
    uint32_t* cr3;
    struct thread_control_block* next;
    uint8_t state;
} thread_control_block;

void initialize_multitasking(void);

