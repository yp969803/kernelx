
[bits 32]

section .text

global isr_ata
extern ata_irq_handler_c

isr_ata:
    pushad
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call ata_irq_handler_c

    pop gs
    pop fs
    pop es
    pop ds
    popad

    ; IRQ14 is on the slave PIC, so EOI must go to both PICs.
    mov al, 0x20
    out 0xA0, al
    out 0x20, al

    iretd
