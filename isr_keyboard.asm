
[bits 32]

global isr1
extern keyboard_handler_c   ; defined in C

isr_keyboard:
    pusha                   ; push eax, ecx, edx, ebx, esp, ebp, esi, edi
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10            ; kernel data segment selector (your DATA_SEG from GDT)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call keyboard_handler_c ; jump into your C handler

    pop gs
    pop fs
    pop es
    pop ds
    popa

    ; Send End Of Interrupt (EOI) to PIC
    mov al, 0x20
    out 0x20, al

    iretd                   ; return from interrupt
