
[bits 32]
global switch_to_task
extern current_task_TCB
extern next_task_TCB
extern tss_entry


%define TCB_ESP      0
%define TCB_ESP0     4
%define TCB_CR3      8

section .text

switch_to_task:
    cli
    pushad
    push ds
    push es
    push fs
    push gs

    mov edi,[current_task_TCB]    
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

    pop ds
    pop es
    pop fs
    pop gs
    popad
    
    sti
    iretd
