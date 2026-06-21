#pragma once

#include <stdint.h>

#define SYS_WRITE 0
#define SYS_EXIT 1
#define SYS_SLEEP 2

typedef int (*syscall_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

typedef struct pt_regs {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp;
    uint32_t ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags, user_esp, ss;
} __attribute__((packed)) pt_regs;
