#include "idt.h"
#include "io.h"
#include "page_fault.h"
#include "divide_zero.h"
#include "../stdlib/stdio.h"
#include "../kernel/utils.h"


// Exceptios isr
extern void exception_isr0(void);
extern void exception_isr1(void);
extern void exception_isr2(void);
extern void exception_isr3(void);
extern void exception_isr4(void);
extern void exception_isr5(void);
extern void exception_isr6(void);
extern void exception_isr7(void);
extern void exception_isr8(void);
extern void exception_isr9(void);
extern void exception_isr10(void);
extern void exception_isr11(void);
extern void exception_isr12(void);
extern void exception_isr13(void);
extern void exception_isr14(void);
extern void exception_isr15(void);
extern void exception_isr16(void);
extern void exception_isr17(void);
extern void exception_isr18(void);
extern void exception_isr19(void);
extern void exception_isr20(void);
extern void exception_isr21(void);
extern void exception_isr22(void);
extern void exception_isr23(void);
extern void exception_isr24(void);
extern void exception_isr25(void);
extern void exception_isr26(void);
extern void exception_isr27(void);
extern void exception_isr28(void);
extern void exception_isr29(void);
extern void exception_isr30(void);
extern void exception_isr31(void);

// ISR for keyboard interrupt
extern void isr_keyboard(void);      // ISR defined in assembly (or change name to isr_keyboard)
extern void isr_pit(void);

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

    // Unmask IRQ0 (timer) and IRQ1 (keyboard)
    outb(0x21, 0xFF & ~(0x01 | 0x02)); 
    outb(0xA1, 0xFF);
}

void idt_init(void) {
    // zero out IDT
    for (int i = 0; i < IDT_SIZE; ++i) {
        set_idt_entry(i, 0, 0, 0);
    }

    // Set exception handlers
    set_idt_entry(0x00, (uint32_t)exception_isr0, KERNEL_CS, 0x8E);
    set_idt_entry(0x01, (uint32_t)exception_isr1, KERNEL_CS, 0x8E);
    set_idt_entry(0x02, (uint32_t)exception_isr2, KERNEL_CS, 0x8E);
    set_idt_entry(0x03, (uint32_t)exception_isr3, KERNEL_CS, 0x8E  );
    set_idt_entry(0x04, (uint32_t)exception_isr4, KERNEL_CS, 0x8E);
    set_idt_entry(0x05, (uint32_t)exception_isr5, KERNEL_CS, 0x8E);
    set_idt_entry(0x06, (uint32_t)exception_isr6, KERNEL_CS, 0x8E);
    set_idt_entry(0x07, (uint32_t)exception_isr7, KERNEL_CS, 0x8E);
    set_idt_entry(0x08, (uint32_t)exception_isr8, KERNEL_CS, 0x8E);
    set_idt_entry(0x09, (uint32_t)exception_isr9, KERNEL_CS, 0x8E);
    set_idt_entry(0x0A, (uint32_t)exception_isr10, KERNEL_CS, 0x8E);            
    set_idt_entry(0x0B, (uint32_t)exception_isr11, KERNEL_CS, 0x8E);
    set_idt_entry(0x0C, (uint32_t)exception_isr12, KERNEL_CS, 0x8E);
    set_idt_entry(0x0D, (uint32_t)exception_isr13, KERNEL_CS, 0x8E);
    set_idt_entry(0x0E, (uint32_t)exception_isr14, KERNEL_CS, 0x8E);    
    set_idt_entry(0x0F, (uint32_t)exception_isr15, KERNEL_CS, 0x8E);
    set_idt_entry(0x10, (uint32_t)exception_isr16, KERNEL_CS, 0x8E);
    set_idt_entry(0x11, (uint32_t)exception_isr17, KERNEL_CS, 0x8E);
    set_idt_entry(0x12, (uint32_t)exception_isr18, KERNEL_CS, 0x8E);
    set_idt_entry(0x13, (uint32_t)exception_isr19, KERNEL_CS, 0x8E);
    set_idt_entry(0x14, (uint32_t)exception_isr20, KERNEL_CS, 0x8E);
    set_idt_entry(0x15, (uint32_t)exception_isr21, KERNEL_CS, 0x8E);
    set_idt_entry(0x16, (uint32_t)exception_isr22, KERNEL_CS, 0x8E);
    set_idt_entry(0x17, (uint32_t)exception_isr23, KERNEL_CS, 0x8E);
    set_idt_entry(0x18, (uint32_t)exception_isr24, KERNEL_CS, 0x8E);
    set_idt_entry(0x19, (uint32_t)exception_isr25, KERNEL_CS, 0x8E);
    set_idt_entry(0x1A, (uint32_t)exception_isr26, KERNEL_CS, 0x8E);
    set_idt_entry(0x1B, (uint32_t)exception_isr27, KERNEL_CS, 0x8E);
    set_idt_entry(0x1C, (uint32_t)exception_isr28, KERNEL_CS, 0x8E);
    set_idt_entry(0x1D, (uint32_t)exception_isr29, KERNEL_CS, 0x8E);
    set_idt_entry(0x1E, (uint32_t)exception_isr30, KERNEL_CS, 0x8E);
    set_idt_entry(0x1F, (uint32_t)exception_isr31, KERNEL_CS, 0x8E);


    // Set IRQ handlers
    set_idt_entry(0x21, (uint32_t)isr_keyboard, KERNEL_CS, 0x8E);
    set_idt_entry(0x20, (uint32_t)isr_pit, KERNEL_CS, 0x8E);

    // finally load the IDT
    load_idt();
    pic_remap();  // Master at 0x20–0x27, Slave at 0x28–0x2F
    set_interrupt();
}

void exception_handler_c(uint32_t error_code,uint32_t exception_no ) {
    if(exception_no<0 || exception_no >31){
        return;
    }

    if(exception_no == 0x0E){
        // Page Fault
        page_fault_handler_c(error_code);
    }

    if(exception_no == 0x00){
        // Divide by zero
        divide_zero_handler_c();
    }
    kprintf("Exception %d occurred with error code: %d\n", exception_no, error_code);
    panic();
}
