
[bits 32]

global switch_to_task
extern current_task_TCB
extern next_task_TCB
extern tss_entry

global isr_pit
extern pit_handler_c   ; defined in C


%define TCB_ESP      0
%define TCB_ESP0     4
%define TCB_CR3      8

section .text


isr_pit:
    pushad                   ; push eax, ecx, edx, ebx, esp, ebp, esi, edi
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10            ; kernel data segment selector (your DATA_SEG from GDT)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call  pit_handler_c 

    jmp .switch_task


.switch_task:

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
    ; Send End Of Interrupt (EOI) to PIC
    mov al, 0x20
    out 0x20, al

    iretd
