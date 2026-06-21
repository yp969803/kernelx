#include "syscall.h"
#include "../drivers/timer.h"
#include "../kernel/utils.h"
#include "../stdlib/stdio.h"
#include "task.h"

int sys_write(uint32_t buf, uint32_t len, uint32_t unused2, uint32_t unused3, uint32_t unused4,
              uint32_t unused5)
{

    const char *buffer = (const char *)buf;
    if (!buffer) {
        return ERR;
    }

    for (uint32_t i = 0; i < len; i++) {
        kprintf("%c", buffer[i]);
    }

    return (int)len;
}

int sys_exit(uint32_t code, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4,
             uint32_t unused5)
{
    (void)code;
    if (current_task_TCB) {
        current_task_TCB->state = TASK_ZOOMBIE;
        task_pick_next();
    }
    return 0;
}

int sys_sleep(uint32_t milliseconds, uint32_t unused1, uint32_t unused2, uint32_t unused3,
              uint32_t unused4, uint32_t unused5)
{
    if (sleep_current_task(milliseconds) != OK) {
        return ERR;
    }

    task_pick_next();
    return OK;
}

syscall_t syscall_table[] = {
    (syscall_t)sys_write,
    (syscall_t)sys_exit,
    (syscall_t)sys_sleep,
};

void syscall_handler_c(pt_regs *regs)
{
    uint32_t num  = regs->eax; // syscall number
    uint32_t arg1 = regs->ebx;
    uint32_t arg2 = regs->ecx;
    uint32_t arg3 = regs->edx;
    uint32_t arg4 = regs->esi;
    uint32_t arg5 = regs->edi;
    uint32_t arg6 = regs->ebp;

    if (num >= sizeof(syscall_table) / sizeof(syscall_t)) {
        regs->eax = -1; // invalid syscall
        return;
    }

    syscall_t func = syscall_table[num];
    regs->eax      = func(arg1, arg2, arg3, arg4, arg5, arg6);
}
