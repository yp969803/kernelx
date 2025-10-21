#include "../drivers/vga.h"

void page_fault_handler_c(void){
    vga_print_string("Page Fault Exception Occurred!\n");
}