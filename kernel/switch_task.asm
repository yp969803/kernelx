; switch_to_task.asm
; -------------------
; Performs a context switch between threads.
; void switch_to_task(thread_control_block* next_thread);

[bits 32]
global switch_to_task
extern current_task_TCB
extern tss_entry   ; Your tss_entry symbol should point to the CPU tss_entry structure in memory

; Define offsets within the thread_control_block struct
%define TCB_ESP      0
%define TCB_ESP0     4
%define TCB_CR3      8
%define TCB_NEXT     12
%define TCB_STATE    16

section .text

switch_to_task:
    ; cdecl: arguments on stack
    ; [esp + 0] = return address
    ; [esp + 4] = next_thread pointer

    cli                    ; disable interrupts during context switch

    ; Save current task's kernel stack pointer
    push ebx
    push esi
    push edi
    push ebp

    mov edi, [current_task_TCB]     ; EDI = current thread_control_block
    test edi, edi
    jz .load_new_task               ; if no current task (first switch), skip save

    mov [edi + TCB_ESP], esp        ; Save ESP into current task's TCB

.load_new_task:
    ; Get the next task's TCB pointer (argument)
    mov esi, [esp + (4*5)]          ; 4 pushed regs + ret addr + 1st arg = esp + 20
    mov [current_task_TCB], esi     ; Update current_task_TCB = next_thread

    ; Load new task's kernel stack
    mov esp, [esi + TCB_ESP]        ; Load saved ESP

    ; Load new task’s address space (CR3)
    mov eax, [esi + TCB_CR3]        ; eax = new task’s page directory
    mov ecx, cr3                    ; ecx = current CR3
    cmp eax, ecx
    je .skip_cr3_reload
    mov cr3, eax                    ; switch address space (flushes TLB)
.skip_cr3_reload:

    ; Update kernel stack (ESP0) in tss_entry for privilege-level changes
    mov ebx, [esi + TCB_ESP0]       ; ebx = new kernel stack top
    mov [tss_entry + 4], ebx              ; assuming ESP0 is at offset 4 in your tss_entry struct

    ; Restore callee-saved registers
    pop ebp
    pop edi
    pop esi
    pop ebx

    sti                             ; re-enable interrupts
    ret                             ; return to next thread’s EIP (saved on stack)
