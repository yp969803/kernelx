#pragma once

#include "task.h"
#include <stdint.h>

typedef struct mutex_wait_node {
    thread_control_block *task;
    struct mutex_wait_node *next;
} mutex_wait_node;

typedef struct mutex {
    uint32_t count;
    thread_control_block *owner;
    mutex_wait_node *wait_queue_head;
    mutex_wait_node *wait_queue_tail;
} mutex;

typedef struct spin_lock {
    uint32_t count;
    thread_control_block *owner;
} spin_lock;

mutex *new_mutex(void);
void mutex_init(mutex *m);
void mutex_lock(mutex *m);
void mutex_unlock(mutex *m);
spin_lock *new_spin_lock(void);
void spin_lock_init(spin_lock *m);
void spin_lock_lock(spin_lock *m);
void spin_lock_unlock(spin_lock *m);