#include "task.h"
#include "../cpu/idt.h"
#include "kmalloc.h"
#include "mem.h"
#include "mutex.h"
#include "process.h"
#include <stddef.h>

extern void switch_to_task();
extern void push_interrupt_frame();

static spinlock task_lock;

thread_control_block *current_task_TCB = NULL;
thread_control_block *task_list_head   = NULL;
thread_control_block *next_task_TCB    = NULL;

static uint32_t next_id = 2;
static volatile uint32_t scheduler_disable_count;

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

void initialize_multitasking(void)
{
    current_task_TCB = kmalloc(sizeof(thread_control_block));
    mem_set(current_task_TCB, 0, sizeof(thread_control_block));

    current_task_TCB->esp = get_esp();
    current_task_TCB->cr3 = get_cr3();
    current_task_TCB->process = process_create_kernel(current_task_TCB->cr3);

    current_task_TCB->state             = TASK_RUNNING;
    current_task_TCB->next              = current_task_TCB;
    task_list_head                      = current_task_TCB;
    current_task_TCB->id                = 1;
    current_task_TCB->pid               = current_task_TCB->process ? current_task_TCB->process->pid
                                                                    : KERNEL_PID;
    current_task_TCB->kernel_stack_base = NULL;
    current_task_TCB->time_used         = 0;
    current_task_TCB->time_quantum      = TIME_QUANTUM_MS;
    current_task_TCB->type              = TASK_KERNEL;
    next_task_TCB                       = current_task_TCB;
    spinlock_init(&task_lock);
}

thread_control_block *create_kernel_task(void *(*entry_point)(void *), void *args)
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
    *(--stack_top) = (uint32_t)KERNEL_DS;
    *(--stack_top) = (uint32_t)KERNEL_DS;
    *(--stack_top) = (uint32_t)KERNEL_DS;
    *(--stack_top) = (uint32_t)KERNEL_DS;

    tcb->esp               = stack_top;
    tcb->esp0              = stack_top;
    tcb->cr3               = INIT_PAGE_DIR_PHY;
    tcb->process           = current_task_TCB->process;
    tcb->state             = TASK_READY;
    tcb->kernel_stack_base = stack;
    tcb->type              = TASK_KERNEL;
    tcb->time_used         = 0;
    tcb->time_quantum      = TIME_QUANTUM_MS;

    spinlock_lock(&task_lock);
    tcb->id  = next_id++;
    process_retain(tcb->process);
    tcb->pid = tcb->process ? tcb->process->pid : KERNEL_PID;

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

thread_control_block *create_user_task(void *(*entry_point)(void *), uint32_t *user_stack_base,
                                       uint32_t *user_stack_top, uint32_t *page_dir,
                                       uint32_t *page_dir_phys)
{

    thread_control_block *tcb = kmalloc(sizeof(thread_control_block));
    mem_set(tcb, 0, sizeof(thread_control_block));

    uint32_t *kernel_stack     = kmalloc(KERNEL_STACK_SIZE);
    uint32_t *kernel_stack_top = kernel_stack + KERNEL_STACK_SIZE / 4;

    *(--kernel_stack_top) = (uint32_t)USER_DS;
    *(--kernel_stack_top) = (uint32_t)user_stack_top;
    *(--kernel_stack_top) = (uint32_t)INITIAL_EFLAGS;
    *(--kernel_stack_top) = (uint32_t)USER_CS;
    *(--kernel_stack_top) = (uint32_t)entry_point; // EIP
    // pushad
    *(--kernel_stack_top) = 0;
    *(--kernel_stack_top) = 0;
    *(--kernel_stack_top) = 0;
    *(--kernel_stack_top) = 0;
    *(--kernel_stack_top) = 0;
    *(--kernel_stack_top) = 0;
    *(--kernel_stack_top) = 0;
    *(--kernel_stack_top) = 0;

    // segments
    *(--kernel_stack_top) = (uint32_t)USER_DS;
    *(--kernel_stack_top) = (uint32_t)USER_DS;
    *(--kernel_stack_top) = (uint32_t)USER_DS;
    *(--kernel_stack_top) = (uint32_t)USER_DS;

    tcb->esp               = kernel_stack_top;
    tcb->esp0              = kernel_stack + KERNEL_STACK_SIZE / 4;
    tcb->cr3               = page_dir_phys;
    tcb->process           = process_create_user(page_dir, page_dir_phys);
    if (!tcb->process) {
        kfree(kernel_stack);
        kfree(tcb);
        return NULL;
    }
    tcb->state             = TASK_READY;
    tcb->kernel_stack_base = kernel_stack;
    tcb->user_stack_base   = user_stack_base;
    tcb->type              = TASK_USER;
    tcb->time_used         = 0;
    tcb->time_quantum      = TIME_QUANTUM_MS;

    spinlock_lock(&task_lock);
    tcb->id  = next_id++;
    tcb->pid = tcb->process->pid;

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

void task_pick_next(void)
{
    spinlock_lock(&task_lock);
    set_next_task();
    spinlock_unlock(&task_lock);
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

    if (current_task_TCB->type == TASK_USER) {
        switch_to_task();
    } else {
        push_interrupt_frame();
        switch_to_task();
    }
}

void task_scheduler_disable(void)
{
    scheduler_disable_count++;
}

void task_scheduler_enable(void)
{
    if (scheduler_disable_count > 0) {
        scheduler_disable_count--;
    }
}

void exit(void)
{
    if (!current_task_TCB) {
        return;
    }
    current_task_TCB->state = TASK_ZOOMBIE;
    schedule();
}

void quantum_expired_handler(void)
{
    if (!current_task_TCB) {
        return;
    }
    if (scheduler_disable_count > 0) {
        next_task_TCB = current_task_TCB;
        return;
    }

    current_task_TCB->time_used++;
    current_task_TCB->time_quantum--;
    if (current_task_TCB->time_quantum <= 0 || current_task_TCB->state != TASK_RUNNING) {
        set_next_task();
    } else {
        next_task_TCB = current_task_TCB;
    }
}
