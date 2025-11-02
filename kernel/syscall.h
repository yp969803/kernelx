#pragma once

#include <stdint.h>

typedef struct pt_regs {
    uint32_t gs, fs, es, ds;              
    uint32_t edi, esi, ebp, esp;
    uint32_t ebx, edx, ecx, eax; 
    uint32_t eip, cs, eflags, user_esp, ss; 
} __attribute__((packed)) pt_regs;