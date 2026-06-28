[bits 32]

global isr_syscall
extern syscall_handler_c   
extern current_task_TCB
extern next_task_TCB
extern tss_entry

%define TCB_ESP      0
%define TCB_ESP0     4
%define TCB_CR3      8
%define TCB_STATE    20
%define TASK_RUNNING 1

section .text

isr_syscall:
    cli
    pushad
    xor eax, eax
    mov ax, ds
    push eax
    mov ax, es
    push eax
    mov ax, fs
    push eax
    mov ax, gs
    push eax

    mov ax, 0x10            ; kernel data segment selector 
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call syscall_handler_c 

    add esp, 4

    mov edi,[current_task_TCB]
    cmp dword [edi+TCB_STATE], TASK_RUNNING
    je .doneVAS

    mov esi,[next_task_TCB]
    cmp edi, esi
    je .doneVAS

    mov [edi+TCB_ESP],esp
    mov [current_task_TCB],esi

    mov esp,[esi+TCB_ESP]
    mov eax,[esi+TCB_CR3]
    mov ebx,[esi+TCB_ESP0]
    mov [tss_entry + 4],ebx
    mov ecx,cr3

    cmp eax,ecx
    je .doneVAS
    mov cr3,eax
.doneVAS:

    pop gs
    pop fs
    pop es
    pop ds
    popad
    iretd
