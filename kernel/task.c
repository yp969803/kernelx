#include "task.h"

extern void switch_to_task(thread_control_block* next_thread);
 
struct thread_control_block* current_task_TCB;
