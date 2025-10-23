[bits 32]

DATA_SEG equ 0x10 

%macro ISR_NOERR 1
global exception_isr%1
exception_isr%1:
    push dword 0          ; dummy error code
    push dword %1         ; interrupt number
    jmp common_isr_noerr
%endmacro

%macro ISR_ERR 1
global exception_isr%1
exception_isr%1:
    push dword %1         ; interrupt number
    jmp common_isr_err
%endmacro


extern exception_handler_c   ; defined in C

section .text

common_isr_noerr:
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

    mov eax, [esp + 48]   ; get the exception code
    push eax              ; pass it as argument to C function
    mov eax, [esp + 56]   ; get the error code
    push eax              ; pass it as argument to C function
    call exception_handler_c
    add esp, 8            ; clean up argument

    pop gs
    pop fs
    pop es
    pop ds
    popa

    add esp, 8            ; clean up pushed error code and interrupt number

    iretd                   ; return from interrupt

common_isr_err:
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

    mov eax, [esp + 48]   ; get the exception code
    push eax              ; pass it as argument to C function
    mov eax, [esp + 56]   ; get the error code
    push eax              ; pass it as argument to C function
    call exception_handler_c
    add esp, 8            ; clean up argument

    pop gs
    pop fs
    pop es
    pop ds
    popa

    add esp, 4            ; clean up pushed error code and interrupt number

    iretd                   ; return from interrupt

ISR_NOERR 0 
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR 8
ISR_NOERR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR_NOERR 15
ISR_NOERR 16

ISR_ERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_ERR 29
ISR_ERR 30
ISR_NOERR 31


