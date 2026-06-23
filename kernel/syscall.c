#include "syscall.h"
#include "../drivers/keyboard.h"
#include "../drivers/timer.h"
#include "../drivers/vga.h"
#include "../kernel/utils.h"
#include "../stdlib/stdio.h"
#include "mem.h"
#include "process.h"
#include "task.h"

int sys_write(uint32_t fd, uint32_t buf, uint32_t len, uint32_t unused3, uint32_t unused4,
              uint32_t unused5)
{
    (void)unused3;
    (void)unused4;
    (void)unused5;

    if (len == 0) {
        return 0;
    }

    if (!current_task_TCB || !buf) {
        return ERR;
    }

    file_descriptor_t *desc = process_get_fd(current_task_TCB->process, fd);
    if (!desc || (desc->type != FD_STDOUT && desc->type != FD_STDERR)) {
        return ERR;
    }

    for (uint32_t i = 0; i < len; i++) {
        char ch;
        if (copy_from_user(current_task_TCB->process->page_dir, &ch, buf + i, 1) != OK) {
            return ERR;
        }
        kprintf("%c", ch);
    }

    return (int)len;
}

int sys_read(uint32_t fd, uint32_t buf, uint32_t len, uint32_t unused3, uint32_t unused4,
             uint32_t unused5)
{
    (void)unused3;
    (void)unused4;
    (void)unused5;

    if (len == 0) {
        return 0;
    }
    if (!current_task_TCB || !buf) {
        return ERR;
    }

    file_descriptor_t *desc = process_get_fd(current_task_TCB->process, fd);
    if (!desc || desc->type != FD_STDIN) {
        return ERR;
    }

    if (memValidateUserBuffer(current_task_TCB->process->page_dir, buf, len, true) != OK) {
        return ERR;
    }

    uint32_t count = 0;
    task_scheduler_disable();
    set_interrupt();

    while (count < len) {
        char ch = (char)keyboard_getchar();

        if (ch == '\b') {
            if (count > 0) {
                count--;
                vga_remove_char();
            }
            continue;
        }

        if (copy_to_user(current_task_TCB->process->page_dir, buf + count, &ch, 1) != OK) {
            clear_interrupt();
            task_scheduler_enable();
            return ERR;
        }

        count++;
        if (ch == '\n') {
            vga_put_char('\n');
            break;
        }

        vga_put_char((uint8_t)ch);
    }

    clear_interrupt();
    task_scheduler_enable();
    return (int)count;
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
    (syscall_t)sys_read,
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
