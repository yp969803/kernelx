
[bits 32]
global switch_to_task
global push_interrupt_frame
extern current_task_TCB
extern next_task_TCB
extern tss_entry


%define TCB_ESP      0
%define TCB_ESP0     4
%define TCB_CR3      8

section .text

push_interrupt_frame:
    pop eax
    pushfd
    push cs
    push eax
    ret

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

    pop gs
    pop fs
    pop es
    pop ds
    popad
    
    sti
    iretd
