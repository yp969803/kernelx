[bits 32]

global user_test_start
global user_test_end

%define SYS_WRITE 0
%define SYS_EXIT  1
%define SYS_SLEEP 2

section .text

user_test_start:
    call .base
.base:
    pop esi

    mov eax, SYS_WRITE
    lea ebx, [esi + msg_start - .base]
    mov ecx, msg_start_len
    int 0x80

    mov eax, SYS_SLEEP
    mov ebx, 1000
    int 0x80

    mov eax, SYS_WRITE
    lea ebx, [esi + msg_wake - .base]
    mov ecx, msg_wake_len
    int 0x80

    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

.hang:
    jmp .hang

msg_start:
    db "hello from userspace", 10
msg_start_len equ $ - msg_start

msg_wake:
    db "userspace woke and exiting", 10
msg_wake_len equ $ - msg_wake

user_test_end:

