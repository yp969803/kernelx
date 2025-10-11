[bits 32]

MAGIC_NUMBER equ 0x1BADB002
FLAGS equ (1 << 0) | (1 << 1) 
CHECKSUM equ -(MAGIC_NUMBER + FLAGS)

section .multiboot               
        dd MAGIC_NUMBER            
        dd FLAGS                  
        dd CHECKSUM  

global _start      
extern main        

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