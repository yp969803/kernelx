[bits 32]

global user_test_start
global user_test_end

%define SYS_WRITE 0
%define SYS_EXIT  1
%define SYS_SLEEP 2
%define SYS_READ  3
%define SYS_OPEN  4
%define SYS_CLOSE 5
%define SYS_LSEEK 6

%define O_RDONLY 0x0000
%define O_WRONLY 0x0001
%define O_CREAT  0x0040
%define O_TRUNC  0x0200
%define SEEK_SET 0

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

    mov eax, SYS_OPEN
    lea ebx, [esi + test_path - .base]
    mov ecx, O_WRONLY | O_CREAT | O_TRUNC
    int 0x80
    cmp eax, 0
    jl .file_fail
    mov ebp, eax

    mov eax, SYS_WRITE
    mov ebx, ebp
    lea ecx, [esi + file_data - .base]
    mov edx, file_data_len
    int 0x80

    mov eax, SYS_CLOSE
    mov ebx, ebp
    int 0x80

    mov eax, SYS_OPEN
    lea ebx, [esi + test_path - .base]
    mov ecx, O_RDONLY
    int 0x80
    cmp eax, 0
    jl .file_fail
    mov ebp, eax

    mov eax, SYS_READ
    mov ebx, ebp
    mov ecx, edi
    mov edx, file_data_len
    int 0x80
    cmp eax, file_data_len
    jne .file_close_fail

    mov eax, SYS_WRITE
    mov ebx, 1
    lea ecx, [esi + msg_file_ok - .base]
    mov edx, msg_file_ok_len
    int 0x80

    mov eax, SYS_WRITE
    mov ebx, 1
    mov ecx, edi
    mov edx, file_data_len
    int 0x80

    mov eax, SYS_CLOSE
    mov ebx, ebp
    int 0x80
    jmp .prompt

.file_close_fail:
    mov eax, SYS_CLOSE
    mov ebx, ebp
    int 0x80

.file_fail:
    mov eax, SYS_WRITE
    mov ebx, 1
    lea ecx, [esi + msg_file_fail - .base]
    mov edx, msg_file_fail_len
    int 0x80

.prompt:
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

test_path:
    db "/utest.txt", 0

file_data:
    db "userspace file syscall ok", 10
file_data_len equ $ - file_data

msg_file_ok:
    db "file syscall roundtrip: "
msg_file_ok_len equ $ - msg_file_ok

msg_file_fail:
    db "file syscall roundtrip failed", 10
msg_file_fail_len equ $ - msg_file_fail

msg_prompt:
    db "type something: "
msg_prompt_len equ $ - msg_prompt

msg_echo:
    db "echo: "
msg_echo_len equ $ - msg_echo

user_test_end:
