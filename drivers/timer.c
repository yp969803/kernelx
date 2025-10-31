#include "timer.h"
#include <stddef.h>
#include "../cpu/io.h"
#include "../kernel/task.h"
#include "../stdlib/stdio.h"

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND 0x43
#define PIT_FREQ 1193182

static uint32_t time_elapsed_boot = 0;

static sleep_info *sleep_list_head = NULL;
static sleep_info *sleep_list_tail = NULL;

void pit_set_timer(uint32_t freq)
{
    if (freq == 0)
        return;
    uint16_t count = (uint16_t)(PIT_FREQ / freq);
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, count & 0xFF);
    outb(PIT_CHANNEL0, (count >> 8) & 0xFF);
}

void pit_set_delay(uint32_t freq)
{
    if (freq == 0)
        return;
    uint16_t count = (uint16_t)(PIT_FREQ / freq);
    outb(PIT_COMMAND, 0x38);
    outb(PIT_CHANNEL0, count & 0xFF);
    outb(PIT_CHANNEL0, (count >> 8) & 0xFF);
}

void initialize_timer(void)
{
    pit_set_timer(1000);
    time_elapsed_boot = 0;
}

void sleep_wake(void){
    if(sleep_list_head == NULL) {
        return;
    }
    sleep_info *current = sleep_list_head;
    sleep_info *prev = NULL;
    while(current != NULL) {
        if(current->wake_time <= time_elapsed_boot) {
            current->task->state = TASK_READY;

            if(prev == NULL) {
                sleep_list_head = current->next;
                if(sleep_list_head == NULL) {
                    sleep_list_tail = NULL;
                }
                kfree(current);
                current = sleep_list_head;
            } else {
                prev->next = current->next;
                if(current == sleep_list_tail) {
                    sleep_list_tail = prev;
                }
                kfree(current);
                current = prev->next;
            }
        } else {
            prev = current;
            current = current->next;
        }
    }
}

void pit_handler_c(void)
{
    time_elapsed_boot++;
    sleep_wake();
    quantum_expired_handler();
}

void sleep(uint32_t milliseconds)
{
    if(milliseconds == 0 || current_task_TCB == NULL) {
        return;
    }

    current_task_TCB->state = TASK_BLOCKED;

    uint32_t wake_time = time_elapsed_boot + milliseconds;

    sleep_info *new_sleep = (sleep_info *)kmalloc(sizeof(sleep_info));
    new_sleep->wake_time   = wake_time;
    new_sleep->task       = current_task_TCB;
    new_sleep->next       = NULL;

    if (sleep_list_head == NULL) {
        sleep_list_head = new_sleep;
        sleep_list_tail = new_sleep;
    } else {
        sleep_list_tail->next = new_sleep;
        sleep_list_tail       = new_sleep;
    }
}