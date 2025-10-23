#include "task.h"
#include <stddef.h>
#include "kmalloc.h"
#include "mem.h"

extern void switch_to_task(thread_control_block* next_thread);
 
thread_control_block* current_task_TCB = NULL;
thread_control_block* task_list_head = NULL;

static inline uint32_t* get_esp(){
    uint32_t esp;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    return (uint32_t*)esp;
}

static inline uint32_t* get_cr3(){
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return (uint32_t*)cr3;
}

void initialize_multitasking(void) {
    current_task_TCB = kmalloc(sizeof(thread_control_block));
    memset(current_task_TCB, 0, sizeof(thread_control_block));

    current_task_TCB->esp = get_esp();
    current_task_TCB->cr3 = get_cr3();

    current_task_TCB->state = TASK_RUNNING;
    current_task_TCB->next = current_task_TCB;
    task_list_head = current_task_TCB;
}

thread_control_block* create_task(void (*entry_point)(void), uint32_t* page_dir) {

    thread_control_block* tcb = kmalloc(sizeof(thread_control_block));
    memset(tcb, 0, sizeof(thread_control_block));

    uint32_t* stack = kmalloc(KERNEL_STACK_SIZE);
    uint32_t* stack_top = stack + KERNEL_STACK_SIZE/4;

    *(--stack_top) = (uint32_t)entry_point;  // EIP
    *(--stack_top) = 0x08;                   // CS
    *(--stack_top) = 0x202;                  // EFLAGS (IF=1)
    *(--stack_top) = 0;                      // EAX
    *(--stack_top) = 0;                      // ECX
    *(--stack_top) = 0;                      // EDX
    *(--stack_top) = 0;                      // EBX
    *(--stack_top) = 0;                      // EBP
    *(--stack_top) = 0;                      // ESI
    *(--stack_top) = 0;                      // EDI

    tcb->esp = stack_top;
    tcb->esp0 = stack_top; 
    tcb->cr3 = page_dir;
    tcb->state = TASK_READY;

    if (!task_list_head) {
        task_list_head = tcb;
        tcb->next = tcb;  
    } else {
        thread_control_block* tmp = task_list_head;
        while (tmp->next != task_list_head)
            tmp = tmp->next;
        tmp->next = tcb;
        tcb->next = task_list_head;
    }

    return tcb;
}
