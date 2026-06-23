[bits 32]

global user_test_start
global user_test_end

%define SYS_WRITE 0
%define SYS_EXIT  1
%define SYS_SLEEP 2
%define SYS_READ  3

section .text

user_test_start:
    call .base
.base:
    pop esi
    sub esp, 64
    mov edi, esp

    mov eax, SYS_WRITE
    mov ebx, 1
    lea ecx, [esi + msg_start - .base]
    mov edx, msg_start_len
    int 0x80

    mov eax, SYS_SLEEP
    mov ebx, 1000
    int 0x80

    mov eax, SYS_WRITE
    mov ebx, 1
    lea ecx, [esi + msg_wake - .base]
    mov edx, msg_wake_len
    int 0x80

    mov eax, SYS_WRITE
    mov ebx, 2
    lea ecx, [esi + msg_err - .base]
    mov edx, msg_err_len
    int 0x80

    mov eax, SYS_WRITE
    mov ebx, 1
    lea ecx, [esi + msg_prompt - .base]
    mov edx, msg_prompt_len
    int 0x80

    mov eax, SYS_READ
    xor ebx, ebx
    mov ecx, edi
    mov edx, 63
    int 0x80

    cmp eax, 0
    jle .exit

    mov edx, eax

    mov eax, SYS_WRITE
    mov ebx, 1
    lea ecx, [esi + msg_echo - .base]
    push edx
    mov edx, msg_echo_len
    int 0x80
    pop edx

    mov eax, SYS_WRITE
    mov ebx, 1
    mov ecx, edi
    int 0x80

.exit:
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

msg_err:
    db "stderr fd also works", 10
msg_err_len equ $ - msg_err

msg_prompt:
    db "type something: "
msg_prompt_len equ $ - msg_prompt

msg_echo:
    db "echo: "
msg_echo_len equ $ - msg_echo

user_test_end:
