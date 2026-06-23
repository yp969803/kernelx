#pragma once

#include <stdint.h>

#define KERNEL_PID 0

typedef enum { PROCESS_KERNEL = 0, PROCESS_USER } process_type_t;

typedef struct process {
    uint32_t pid;
    process_type_t type;
    uint32_t *page_dir;
    uint32_t *page_dir_phys;
    uint32_t ref_count;
} process_t;

process_t *process_create_kernel(uint32_t *page_dir_phys);
process_t *process_create_user(uint32_t *page_dir, uint32_t *page_dir_phys);
void process_retain(process_t *process);
void process_release(process_t *process);
