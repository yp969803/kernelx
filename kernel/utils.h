#pragma once

#define CEIL_DIV(a, b) (((a) + (b) - 1) / (b))

static inline void halt(void) {
    __asm__ __volatile__("hlt");
}

static inline void clear_interrupt(void){
    __asm__ __volatile__("cli");
}


static inline void panic(void) {
    
    clear_interrupt();  
    while (1) {
        asm volatile("hlt");  // halt the CPU indefinitely
    }
}

