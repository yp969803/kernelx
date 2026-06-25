#pragma once

#include "../fs/vfs.h"
#include <stdbool.h>
#include <stdint.h>

#define KERNEL_PID 0
#define MAX_FDS 16

typedef enum { PROCESS_KERNEL = 0, PROCESS_USER } process_type_t;
typedef enum { FD_NONE = 0, FD_STDIN, FD_STDOUT, FD_STDERR } fd_type_t;

typedef struct file_descriptor {
    bool used;
    fd_type_t type;
    vfs_file_t *file;
} file_descriptor_t;

typedef struct process {
    uint32_t pid;
    process_type_t type;
    uint32_t *page_dir;
    uint32_t *page_dir_phys;
    uint32_t ref_count;
    file_descriptor_t fds[MAX_FDS];
} process_t;

process_t *process_create_kernel(uint32_t *page_dir_phys);
process_t *process_create_user(uint32_t *page_dir, uint32_t *page_dir_phys);
file_descriptor_t *process_get_fd(process_t *process, uint32_t fd);
vfs_file_t *process_get_file(process_t *process, uint32_t fd);
int process_alloc_fd(process_t *process, vfs_file_t *file);
int process_close_fd(process_t *process, uint32_t fd);
void process_retain(process_t *process);
void process_release(process_t *process);
