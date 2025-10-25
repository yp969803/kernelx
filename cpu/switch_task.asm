
[bits 32]
global switch_to_task
extern current_task_TCB
extern tss_entry


%define TCB_ESP      0
%define TCB_ESP0     4
%define TCB_CR3      8
%define TCB_NEXT     12
%define TCB_STATE    16

section .text

switch_to_task:
    push eax
    push ecx
    push edx
    push ebx
    push esi
    push edi
    push ebp

    mov edi,[current_task_TCB]    
    mov [edi+TCB_ESP],esp

    mov esi,[esp+(7+1)*4]
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

    pop ebp
    pop edi
    pop esi
    pop ebx
    pop edx
    pop ecx
    pop eax

    ret
