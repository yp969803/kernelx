#pragma once

#include <stdbool.h>
#include <stdint.h>

#define ERR -1
#define OK 0

#define CEIL_DIV(a, b) (((a) + (b)-1) / (b))

static inline void halt(void)
{
    __asm__ __volatile__("hlt");
}

static inline void clear_interrupt(void)
{
    __asm__ __volatile__("cli");
}

static inline void panic(void)
{

    clear_interrupt();
    while (1) {
        asm volatile("hlt"); // halt the CPU indefinitely
    }
}

static inline void set_interrupt(void)
{
    __asm__ __volatile__("sti");
}

static inline bool are_interrupts_enabled()
{
    unsigned long flags;
    asm volatile("pushf\n\t"
                 "pop %0"
                 : "=g"(flags));
    return flags & (1 << 9);
}

static inline unsigned long save_irqdisable(void)
{
    unsigned long flags;
    asm volatile("pushf\n\tcli\n\tpop %0" : "=r"(flags) : : "memory");
    return flags;
}

static inline void irqrestore(unsigned long flags)
{
    asm("push %0\n\tpopf" : : "rm"(flags) : "memory", "cc");
}
