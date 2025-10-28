#pragma once

#include <stdint.h>

#define INITIAL_EFLAGS 0x202

#define KERNEL_STACK_SIZE 0x4000 // 16 KB
#define TIME_QUANTUM_MS 5

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
    uint32_t* stack_base;
    uint32_t time_used;  // in ms
    int time_quantum; // in ms
} thread_control_block;

void initialize_multitasking(void);
thread_control_block* create_task(void * (*entry_point) (void), uint32_t* page_dir);
void schedule(void);
void exit(void);
void quantum_expired_handler(void);