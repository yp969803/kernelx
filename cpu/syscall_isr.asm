[bits 32]

global isr_syscall
extern syscall_handler_c   

section .text

isr_syscall:
    pushad
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10            ; kernel data segment selector 
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call syscall_handler_c 

    add esp, 4            

    pop gs
    pop fs
    pop es
    pop ds
    popad

    iretd
