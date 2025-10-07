[bits 32]

global _start      ; make this the entry point for linker
extern main        ; tell assembler that 'main' is in another file

section .text
_start:
    call main      ; call kernel main in C
.hang:
    jmp .hang      ; infinite loop so CPU doesn't run into garbage
