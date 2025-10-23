#include "divide_zero.h"
#include "../stdlib/stdio.h"
#include "../kernel/utils.h"

void divide_zero_handler_c(void){
    kprintf("Divide Zero Exception Occurred\n");
    panic();
}

