#include "mutex.h"
#include "kmalloc.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static inline bool atomic_compare_exchange(uint32_t *ptr, uint32_t *expected, uint32_t desired)
{
    unsigned char success;

    __asm__ volatile("lock cmpxchgl %3, %1\n\t"
                     "sete %0"
                     : "=q"(success), "+m"(*ptr), "+a"(*expected)
                     : "r"(desired)
                     : "memory");

    return success;
}

mutex *new_mutex(void)
{
    mutex *m           = kmalloc(sizeof(mutex));
    m->count           = 1;
    m->owner           = NULL;
    m->wait_queue_head = NULL;
    m->wait_queue_tail = NULL;
    return m;
}

void mutex_init(mutex *m)
{
    m->count           = 1;
    m->owner           = NULL;
    m->wait_queue_head = NULL;
    m->wait_queue_tail = NULL;
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

spinlock *new_spinlock(void)
{
    spinlock *m = kmalloc(sizeof(spinlock));
    m->count    = 1;
    m->owner    = NULL;
    return m;
}

void spinlock_init(spinlock *m)
{
    m->count = 1;
    m->owner = NULL;
}

void spinlock_lock(spinlock *m)
{
    uint32_t expected = 1;
    while (!atomic_compare_exchange(&m->count, &expected, 0));
    m->owner = current_task_TCB;
}

void spinlock_unlock(spinlock *m)
{
    if (m->owner != current_task_TCB) {
        return;
    }
    m->owner = NULL;
    m->count = 1;
}