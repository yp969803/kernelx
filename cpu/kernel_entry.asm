[bits 32]

MAGIC_NUMBER equ 0x1BADB002
FLAGS equ (1 << 0) | (1 << 1) 
CHECKSUM equ -(MAGIC_NUMBER + FLAGS)

section .multiboot     
align 4          
    dd MAGIC_NUMBER            
    dd FLAGS                  
    dd CHECKSUM  
    dd 0, 0, 0, 0   ; header_addr, load_addr, load_end_addr, bss_end_addr

section .bss 
align 16 
stack_bottom:
    resb 16384*8 ; 128KB stack
stack_top:
     

section .boot 

global _start
_start:
    mov ecx, (initial_page_dir - 0xC0000000)
    mov cr3, ecx

    mov ecx, cr4
    or ecx, 0x10
    mov cr4, ecx

    mov ecx, cr0
    or ecx, 0x80000000
    mov cr0, ecx

    jmp higher_half


section .text

extern main 

higher_half:
    cli
    mov ebp, stack_top 
    mov esp, ebp
    push ebx        ; push mb_addr
    push eax        ; push magic
    xor ebp, ebp
    call main      ; call kernel main in C
    hlt


section .data
align 4096 
global initial_page_dir 

initial_page_dir:
    dd 10000011b
    times 768-1 dd 0
    dd (0 << 22) | 10000011b
    dd (1 << 22) | 10000011b
    dd (2 << 22) | 10000011b
    dd (3 << 22) | 10000011b
    times 256-4 dd 0
