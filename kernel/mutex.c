#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "mutex.h"


static inline bool atomic_compare_exchange(uint32_t *ptr, uint32_t *expected, uint32_t desired) {
    unsigned char success;

    __asm__ volatile (
        "lock cmpxchgl %3, %1\n\t"
        "sete %0"                     
        : "=q"(success), "+m"(*ptr), "+a"(*expected)
        : "r"(desired)
        : "memory"
    );

    return success;
}

void mutex_init(mutex *m)
{
    m->count            = 1;
    m->owner            = NULL;
    m->wait_queue_head  = NULL;
    m->wait_queue_tail  = NULL;
}

void mutex_lock(mutex *m)
{
    uint32_t expected = 1;
    while (!atomic_compare_exchange(&m->count, &expected, 0)) {
        expected = 1;

        mutex_wait_node *node = kmalloc(sizeof(mutex_wait_node));
        node->task            = current_task_TCB;
        node->next            = NULL;

        if (m->wait_queue_tail) {
            m->wait_queue_tail->next = node;
            m->wait_queue_tail       = node;
        } else {
            m->wait_queue_head = node;
            m->wait_queue_tail = node;
        }

        current_task_TCB->state = TASK_BLOCKED;
        schedule();
    }
    m->owner = current_task_TCB;
}

void mutex_unlock(mutex *m)
{
    if (m->owner != current_task_TCB) {
        return; 
    }

    if (m->wait_queue_head) {
        mutex_wait_node *node = m->wait_queue_head;
        m->wait_queue_head    = node->next;
        if (m->wait_queue_head == NULL) {
            m->wait_queue_tail = NULL;
        }

        node->task->state = TASK_READY;
        kfree(node);
        m->owner = node->task;
    } else {
        m->owner = NULL;
        m->count = 1;
    }
}