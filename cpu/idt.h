#pragma once

#include <stdint.h>

#define IDT_SIZE 256
#define KERNEL_CS 0x08
#define KERNEL_DS 0x10

struct IDTEntry {
    uint16_t offset_low;  // Lower 16 bits of the ISR's address
    uint16_t selector;    // Segment selector
    uint8_t zero;         // Reserved, set to 0
    uint8_t type_attr;    // Type and attributes
    uint16_t offset_high; // Upper 16 bits of the ISR's address
} __attribute__((packed));

struct IDTPtr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

extern struct IDTEntry idt[IDT_SIZE];

void set_idt_entry(int n, uint32_t isr_address, uint16_t selector, uint8_t type_attr);
void idt_init(void);
