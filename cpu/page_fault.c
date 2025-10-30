#include "../kernel/utils.h"
#include "../stdlib/stdio.h"

static inline uint32_t get_cr2(void) 
{
    uint32_t val;
    asm volatile("mov %%cr2, %0" : "=r"(val)); 
    return val;
}

void page_fault_handler_c(uint32_t error_code) 
{
    uint32_t faulting_address = get_cr2();
    kprintf("Page Fault Exception Occurred, Error Code: %x, Faulting Address %x\n", error_code, faulting_address);
    panic();
}