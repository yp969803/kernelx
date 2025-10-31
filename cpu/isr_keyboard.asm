
[bits 32]

section .text

global isr_keyboard
extern keyboard_handler_c   ; defined in C

isr_keyboard:
    pushad                   ; push eax, ecx, edx, ebx, esp, ebp, esi, edi
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10            ; kernel data segment selector 
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call keyboard_handler_c 

    pop gs
    pop fs
    pop es
    pop ds
    popad

    ; Send End Of Interrupt (EOI) to PIC
    mov al, 0x20
    out 0x20, al

    iretd                  
