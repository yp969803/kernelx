#pragma once

#include <stdint.h>

#define KERNEL_STACK_SIZE 0x4000 // 16 KB

typedef enum {
    TASK_READY = 0,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_ZOOMBIE
} task_state_t;

typedef struct thread_control_block{
    uint32_t* esp;
    uint32_t* esp0;
    uint32_t* cr3;
    struct thread_control_block* next;
    task_state_t state;
    uint32_t id;
    uint32_t pid;
} thread_control_block;

void initialize_multitasking(void);
thread_control_block* create_task(void * (*entry_point) (void), uint32_t* page_dir, uint32_t pid);
void schedule(void);
void exit(void);
