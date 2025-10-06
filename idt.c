#include "idt.h"

// ISR for keyboard interrupt
extern void isr1(void);      // ISR defined in assembly (or change name to isr_keyboard)

struct IDTEntry idt[IDT_SIZE];

static struct IDTPtr idt_ptr;

void set_idt_entry(int n, uint32_t isr_address, uint16_t selector, uint8_t type_attr) {
    idt[n].offset_low  = (uint16_t)(isr_address & 0xFFFF);
    idt[n].selector    = selector;
    idt[n].zero        = 0;
    idt[n].type_attr   = type_attr;
    idt[n].offset_high = (uint16_t)((isr_address >> 16) & 0xFFFF);
}

static inline void load_idt(void) {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint32_t)&idt;
    asm volatile ("lidt %0" : : "m"(idt_ptr));  // load the IDT
}

void idt_init(void) {
    // zero out IDT
    for (int i = 0; i < IDT_SIZE; ++i) {
        set_idt_entry(i, 0, 0, 0);
    }

    set_idt_entry(33, (uint32_t)isr1, KERNEL_CS, 0x8E);

    // finally load the IDT
    load_idt();
}
