#include "idt.h"
#include "io.h"

// ISR for keyboard interrupt
extern void isr_keyboard(void);      // ISR defined in assembly (or change name to isr_keyboard)

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

static inline void pic_remap(void) {

    // Start initialization sequence (cascade mode)
    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    // Set vector offset
    outb(0x21, 0x20);  // Master PIC vector offset
    outb(0xA1, 0x28);  // Slave PIC vector offset

    // Tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    outb(0x21, 0x04);
    outb(0xA1, 0x02);

    // Set environment info
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    // Only unmask keyboard interrupt (IRQ1)
    outb(0x21, 0xFF & ~0x02); // bit 1 = 0 (unmask), others = 1 (masked)
    outb(0xA1, 0xFF);
}

static inline void set_interrupt(void){
    __asm__ __volatile__("sti");
}

void idt_init(void) {
    // zero out IDT
    for (int i = 0; i < IDT_SIZE; ++i) {
        set_idt_entry(i, 0, 0, 0);
    }

    set_idt_entry(0x21, (uint32_t)isr_keyboard, KERNEL_CS, 0x8E);

    // finally load the IDT
    load_idt();
    pic_remap();  // Master at 0x20–0x27, Slave at 0x28–0x2F
    set_interrupt();
}

