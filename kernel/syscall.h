#pragma once

#include <stdint.h>

#define SYS_WRITE 0
#define SYS_EXIT 1
#define SYS_SLEEP 2
#define SYS_READ 3
#define SYS_OPEN 4
#define SYS_CLOSE 5
#define SYS_LSEEK 6
#define SYS_UNLINK 7
#define SYS_MKDIR 8

typedef int (*syscall_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

typedef struct pt_regs {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp;
    uint32_t ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags, user_esp, ss;
} __attribute__((packed)) pt_regs;
