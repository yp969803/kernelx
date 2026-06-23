#include "process.h"
#include "kmalloc.h"
#include "mem.h"

static uint32_t next_pid = 1;

static process_t *process_create(process_type_t type, uint32_t pid, uint32_t *page_dir,
                                 uint32_t *page_dir_phys)
{
    process_t *process = kmalloc(sizeof(process_t));
    if (!process) {
        return 0;
    }

    mem_set(process, 0, sizeof(process_t));
    process->pid           = pid;
    process->type          = type;
    process->page_dir      = page_dir;
    process->page_dir_phys = page_dir_phys;
    process->ref_count     = 1;
    return process;
}

process_t *process_create_kernel(uint32_t *page_dir_phys)
{
    return process_create(PROCESS_KERNEL, KERNEL_PID, initial_page_dir, page_dir_phys);
}

process_t *process_create_user(uint32_t *page_dir, uint32_t *page_dir_phys)
{
    return process_create(PROCESS_USER, next_pid++, page_dir, page_dir_phys);
}

void process_retain(process_t *process)
{
    if (!process) {
        return;
    }

    process->ref_count++;
}

void process_release(process_t *process)
{
    if (!process || process->ref_count == 0) {
        return;
    }

    process->ref_count--;
    if (process->ref_count == 0) {
        kfree(process);
    }
}
