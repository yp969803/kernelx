#include "task.h"
#include "../cpu/idt.h"
#include "kmalloc.h"
#include "mem.h"
#include "mutex.h"
#include <stddef.h>

extern void switch_to_task();

static spinlock task_lock;

thread_control_block *current_task_TCB = NULL;
thread_control_block *task_list_head   = NULL;
thread_control_block *next_task_TCB    = NULL;

static uint32_t next_id = 2;

static inline uint32_t *get_esp()
{
    uint32_t esp;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    return (uint32_t *)esp;
}

static inline uint32_t *get_cr3()
{
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return (uint32_t *)cr3;
}

static inline void push_interrupt_frame(void)
{
    asm volatile("pushfl\n\t" // push EFLAGS
                 "mov %%cs, %%ax\n\t"
                 "push %%eax\n\t" // push CS
                 :
                 :
                 : "ax", "memory");
}

void initialize_multitasking(void)
{
    current_task_TCB = kmalloc(sizeof(thread_control_block));
    mem_set(current_task_TCB, 0, sizeof(thread_control_block));

    current_task_TCB->esp = get_esp();
    current_task_TCB->cr3 = get_cr3();

    current_task_TCB->state        = TASK_RUNNING;
    current_task_TCB->next         = current_task_TCB;
    task_list_head                 = current_task_TCB;
    current_task_TCB->id           = 1;
    current_task_TCB->pid          = 0;
    current_task_TCB->stack_base   = NULL;
    current_task_TCB->time_used    = 0;
    current_task_TCB->time_quantum = TIME_QUANTUM_MS;

    next_task_TCB = current_task_TCB;
    spinlock_init(&task_lock);
}

thread_control_block *create_task(void *(*entry_point)(void *), void *args, uint32_t *page_dir)
{

    thread_control_block *tcb = kmalloc(sizeof(thread_control_block));
    mem_set(tcb, 0, sizeof(thread_control_block));

    uint32_t *stack     = kmalloc(KERNEL_STACK_SIZE);
    uint32_t *stack_top = stack + KERNEL_STACK_SIZE / 4;

    *(--stack_top) = (uint32_t)args;
    *(--stack_top) = 0; // fake return address
    *(--stack_top) = (uint32_t)INITIAL_EFLAGS;
    *(--stack_top) = (uint32_t)KERNEL_CS;
    *(--stack_top) = (uint32_t)entry_point; // EIP
    // pushad
    *(--stack_top) = 0;
    *(--stack_top) = 0;
    *(--stack_top) = 0;
    *(--stack_top) = 0;
    *(--stack_top) = 0;
    *(--stack_top) = 0;
    *(--stack_top) = 0;
    *(--stack_top) = 0;

    // segments
    *(--stack_top) = 0x10;
    *(--stack_top) = 0x10;
    *(--stack_top) = 0x10;
    *(--stack_top) = 0x10;

    tcb->esp          = stack_top;
    tcb->esp0         = stack_top;
    tcb->cr3          = page_dir;
    tcb->state        = TASK_READY;
    tcb->stack_base   = stack;
    tcb->time_used    = 0;
    tcb->time_quantum = TIME_QUANTUM_MS;

    spinlock_lock(&task_lock);
    tcb->id  = next_id++;
    tcb->pid = current_task_TCB->pid;

    if (!task_list_head) {
        task_list_head = tcb;
        tcb->next      = tcb;
    } else {
        thread_control_block *tmp = task_list_head;
        while (tmp->next != task_list_head)
            tmp = tmp->next;
        tmp->next = tcb;
        tcb->next = task_list_head;
    }
    spinlock_unlock(&task_lock);

    return tcb;
}

void set_next_task()
{
    current_task_TCB->time_quantum = TIME_QUANTUM_MS;
    thread_control_block *next     = current_task_TCB->next;
    if (next == current_task_TCB) {
        next_task_TCB = current_task_TCB;
        return;
    }

    while (next->state != TASK_READY) {
        next = next->next;
        if (next == current_task_TCB) {
            next_task_TCB = current_task_TCB;
            return;
        }
    }

    if (current_task_TCB->state == TASK_RUNNING) {
        current_task_TCB->state = TASK_READY;
    }

    next->state = TASK_RUNNING;

    next_task_TCB = next;
}

void schedule(void)
{
    if (!current_task_TCB || !current_task_TCB->next) {
        return;
    }

    spinlock_lock(&task_lock);

    set_next_task();

    if (current_task_TCB == next_task_TCB) {
        spinlock_unlock(&task_lock);
        return;
    }

    spinlock_unlock(&task_lock);
    push_interrupt_frame();
    switch_to_task();
}

void exit(void)
{
    if (!current_task_TCB) {
        return;
    }
    spinlock_lock(&task_lock);
    current_task_TCB->state = TASK_ZOOMBIE;
    spinlock_unlock(&task_lock);
    schedule();
}

void quantum_expired_handler(void)
{
    if (!current_task_TCB) {
        return;
    }
    spinlock_lock(&task_lock);
    current_task_TCB->time_used++;
    current_task_TCB->time_quantum--;
    if (current_task_TCB->time_quantum <= 0 || current_task_TCB->state != TASK_RUNNING) {
        set_next_task();
    } else {
        next_task_TCB = current_task_TCB;
    }
    spinlock_unlock(&task_lock);
}