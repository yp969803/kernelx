[bits 32]


section .multiboot               ;according to multiboot spec
        dd 0x1BADB002            ;set magic number for
                                 ;bootloader
        dd 0x1                   ;set flags
        dd - (0x1BADB002 + 0x1)  ;set checksum

global _start      ; make this the entry point for linker
extern main        ; tell assembler that 'main' is in another file

section .text
_start:
    cli
    mov ebp, stack_space 
    mov esp, ebp
    push ebx        ; push mb_addr
    push eax        ; push magic
    call main      ; call kernel main in C
    hlt

section .bss
resb 16000                        ;16KB for stack
stack_space: