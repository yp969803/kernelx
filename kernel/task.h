#pragma once

#include <stdint.h>

#define INITIAL_EFLAGS 0x202

#define KERNEL_STACK_SIZE 0x4000 // 16 KB
#define TIME_QUANTUM_MS 5

typedef enum { TASK_READY = 0, TASK_RUNNING, TASK_BLOCKED, TASK_ZOOMBIE } task_state_t;
typedef enum { TASK_KERNEL = 0, TASK_USER } task_type_t;

typedef struct process process_t;

typedef struct thread_control_block {
    uint32_t *esp;
    uint32_t *esp0;
    uint32_t *cr3;
    process_t *process;
    struct thread_control_block *next;
    task_state_t state;
    uint32_t id;
    uint32_t pid;
    uint32_t *kernel_stack_base;
    uint32_t *user_stack_base;
    uint32_t time_used; // in ms
    int time_quantum;   // in ms
    task_type_t type;
} thread_control_block;

extern thread_control_block *current_task_TCB;

void initialize_multitasking(void);
thread_control_block *create_kernel_task(void *(*entry_point)(void *), void *args);
thread_control_block *create_user_task(void *(*entry_point)(void *), uint32_t *user_stack_base,
                                       uint32_t *user_stack_top, uint32_t *page_dir,
                                       uint32_t *page_dir_phys);
void task_pick_next(void);
void schedule(void);
void task_scheduler_disable(void);
void task_scheduler_enable(void);
void exit(void);
void quantum_expired_handler(void);
