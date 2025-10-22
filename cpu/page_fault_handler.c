#include "../drivers/vga.h"
#include "../kernel/utils.h"

static inline uint32_t get_cr2(void) {
    uint32_t val;
    asm volatile("mov %%cr2, %0" : "=r"(val)); 
    return val;
}

void page_fault_handler_c(uint32_t error_code) {
    vga_print_string("Page Fault Exception Occurred!\n");
    vga_print_string("Faulting Address: ");
    uint32_t faulting_address = get_cr2();
    vga_print_hex(faulting_address);
    vga_print_string("\nError Code: ");
    vga_print_hex(error_code);
    vga_print_string("\n");
    panic();
}