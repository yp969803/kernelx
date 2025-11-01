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

typedef struct spinlock {
    uint32_t count;
    thread_control_block *owner;
} spinlock;

mutex *new_mutex(void);
void mutex_init(mutex *m);
void mutex_lock(mutex *m);
void mutex_unlock(mutex *m);
spinlock *new_spinlock(void);
void spinlock_init(spinlock *m);
void spinlock_lock(spinlock *m);
void spinlock_unlock(spinlock *m);