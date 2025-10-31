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

void mutex_init(mutex *m);
void mutex_lock(mutex *m);
void mutex_unlock(mutex *m);