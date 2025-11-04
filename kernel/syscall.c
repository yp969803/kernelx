#include "syscall.h"
#include "../stdlib/stdio.h"

int sys_write(uint32_t buf, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4,
              uint32_t unused5)
{

    const char *buffer = (const char *)buf;

    int i = 0;
    while (buffer[i] != '\0') {
        kprintf("%c", buffer[i]);
    }

    return 1;
}

syscall_t syscall_table[] = {
    (syscall_t)sys_write,
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
