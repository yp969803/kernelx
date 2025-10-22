[bits 32]

DATA_SEG equ 0x10 

section .text

global page_fault
extern page_fault_handler_c   ; defined in C

page_fault:
    pusha                   ; push eax, ecx, edx, ebx, esp, ebp, esi, edi
    push ds
    push es
    push fs
    push gs

    mov ax, DATA_SEG            ; kernel data segment selector (your DATA_SEG from GDT)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call page_fault_handler_c ; jump into your C handler

    pop gs
    pop fs
    pop es
    pop ds
    popa

    iretd                   ; return from interrupt
