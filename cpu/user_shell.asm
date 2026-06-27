[bits 32]

global user_shell_start
global user_shell_end

%define SYS_WRITE    0
%define SYS_EXIT     1
%define SYS_READ     3
%define SYS_OPEN     4
%define SYS_CLOSE    5
%define SYS_UNLINK   7
%define SYS_MKDIR    8
%define SYS_GETDENTS 9

%define O_RDONLY    0x0000
%define O_WRONLY    0x0001
%define O_CREAT     0x0040
%define O_TRUNC     0x0200
%define O_DIRECTORY 0x10000

%define LINE_SIZE   256
%define DIRENT_SIZE 264
%define SAVE_CMD    900
%define SAVE_FD     904
%define SAVE_PATH   908

section .text

user_shell_start:
    call .base
.base:
    pop esi
    sub esp, 1024
    
    lea ecx, [esi + msg_start - .base]
    call print_cstr

.loop:
    lea ecx, [esi + prompt - .base]
    call print_cstr

    mov eax, SYS_READ
    xor ebx, ebx
    mov ecx, esp
    mov edx, LINE_SIZE - 1
    int 0x80
    cmp eax, 0
    jle .loop

    mov edi, esp
    call trim_line
    mov edi, esp
    call next_token
    test eax, eax
    jz .loop
    mov [esp + SAVE_CMD], eax

    mov ebx, [esp + SAVE_CMD]
    lea ecx, [esi + cmd_help - .base]
    call token_eq
    cmp eax, 1
    je .do_help

    mov ebx, [esp + SAVE_CMD]
    lea ecx, [esi + cmd_exit - .base]
    call token_eq
    cmp eax, 1
    je .do_exit

    mov ebx, [esp + SAVE_CMD]
    lea ecx, [esi + cmd_ls - .base]
    call token_eq
    cmp eax, 1
    je .do_ls

    mov ebx, [esp + SAVE_CMD]
    lea ecx, [esi + cmd_cat - .base]
    call token_eq
    cmp eax, 1
    je .do_cat

    mov ebx, [esp + SAVE_CMD]
    lea ecx, [esi + cmd_mkdir - .base]
    call token_eq
    cmp eax, 1
    je .do_mkdir

    mov ebx, [esp + SAVE_CMD]
    lea ecx, [esi + cmd_touch - .base]
    call token_eq
    cmp eax, 1
    je .do_touch

    mov ebx, [esp + SAVE_CMD]
    lea ecx, [esi + cmd_rm - .base]
    call token_eq
    cmp eax, 1
    je .do_rm

    mov ebx, [esp + SAVE_CMD]
    lea ecx, [esi + cmd_write - .base]
    call token_eq
    cmp eax, 1
    je .do_write

    lea ecx, [esi + msg_unknown - .base]
    call print_cstr
    jmp .loop

.do_help:
    lea ecx, [esi + msg_help - .base]
    call print_cstr
    jmp .loop

.do_exit:
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80
    jmp $

.do_ls:
    call next_token
    test eax, eax
    jnz .ls_open
    lea eax, [esi + root_path - .base]
.ls_open:
    mov ebx, eax
    mov eax, SYS_OPEN
    mov ecx, O_RDONLY | O_DIRECTORY
    int 0x80
    cmp eax, 0
    jl .cmd_failed
    mov [esp + SAVE_FD], eax

.ls_read:
    mov byte [esp + LINE_SIZE], 0
    mov eax, SYS_GETDENTS
    mov ebx, [esp + SAVE_FD]
    lea ecx, [esp + LINE_SIZE]
    mov edx, DIRENT_SIZE
    int 0x80
    cmp byte [esp + LINE_SIZE], 0
    je .ls_close

    lea ecx, [esp + LINE_SIZE]
    call print_cstr
    lea ecx, [esi + newline - .base]
    call print_cstr
    jmp .ls_read

.ls_close:
    mov eax, SYS_CLOSE
    mov ebx, [esp + SAVE_FD]
    int 0x80
    jmp .loop

.do_cat:
    call next_token
    test eax, eax
    jz .missing_operand

    mov ebx, eax
    mov eax, SYS_OPEN
    mov ecx, O_RDONLY
    int 0x80
    cmp eax, 0
    jl .cmd_failed
    mov [esp + SAVE_FD], eax

.cat_read:
    mov eax, SYS_READ
    mov ebx, [esp + SAVE_FD]
    mov ecx, esp
    mov edx, LINE_SIZE
    int 0x80
    cmp eax, 0
    jl .cat_close_fail
    je .cat_close

    mov edx, eax
    mov eax, SYS_WRITE
    mov ebx, 1
    mov ecx, esp
    int 0x80
    jmp .cat_read

.cat_close_fail:
    mov eax, SYS_CLOSE
    mov ebx, [esp + SAVE_FD]
    int 0x80
    jmp .cmd_failed

.cat_close:
    mov eax, SYS_CLOSE
    mov ebx, [esp + SAVE_FD]
    int 0x80
    lea ecx, [esi + newline - .base]
    call print_cstr
    jmp .loop

.do_mkdir:
    call next_token
    test eax, eax
    jz .missing_operand

    mov ebx, eax
    mov eax, SYS_MKDIR
    int 0x80
    cmp eax, 0
    jl .cmd_failed
    jmp .ok

.do_touch:
    call next_token
    test eax, eax
    jz .missing_operand

    mov ebx, eax
    mov eax, SYS_OPEN
    mov ecx, O_WRONLY | O_CREAT
    int 0x80
    cmp eax, 0
    jl .cmd_failed
    mov ebx, eax
    mov eax, SYS_CLOSE
    int 0x80
    jmp .ok

.do_rm:
    call next_token
    test eax, eax
    jz .missing_operand

    mov ebx, eax
    mov eax, SYS_UNLINK
    int 0x80
    cmp eax, 0
    jl .cmd_failed
    jmp .ok

.do_write:
    call next_token
    test eax, eax
    jz .missing_operand
    mov [esp + SAVE_PATH], eax

    call skip_spaces
    test edi, edi
    jz .missing_operand
    cmp byte [edi], 0
    je .missing_operand

    push edi
    mov eax, SYS_OPEN
    mov ebx, [esp + SAVE_PATH + 4]
    mov ecx, O_WRONLY | O_CREAT | O_TRUNC
    int 0x80
    pop edi
    cmp eax, 0
    jl .cmd_failed
    mov [esp + SAVE_FD], eax

    mov ecx, edi
    call strlen
    mov eax, SYS_WRITE
    mov ebx, [esp + SAVE_FD]
    mov ecx, edi
    int 0x80

    mov eax, SYS_CLOSE
    mov ebx, [esp + SAVE_FD]
    int 0x80
    jmp .ok

.missing_operand:
    lea ecx, [esi + msg_missing - .base]
    call print_cstr
    jmp .loop

.cmd_failed:
    lea ecx, [esi + msg_failed - .base]
    call print_cstr
    jmp .loop

.ok:
    lea ecx, [esi + msg_ok - .base]
    call print_cstr
    jmp .loop

trim_line:
    mov ecx, eax
    mov ebx, edi
.trim_next:
    cmp ecx, 0
    je .trim_end
    cmp byte [ebx], 10
    je .trim_zero
    cmp byte [ebx], 13
    je .trim_zero
    inc ebx
    dec ecx
    jmp .trim_next
.trim_zero:
    mov byte [ebx], 0
    ret
.trim_end:
    mov byte [ebx], 0
    ret

skip_spaces:
    cmp byte [edi], ' '
    jne .skip_done
    inc edi
    jmp skip_spaces
.skip_done:
    mov eax, edi
    ret

next_token:
    call skip_spaces
    cmp byte [edi], 0
    je .no_token
    mov eax, edi
.token_scan:
    cmp byte [edi], 0
    je .token_done
    cmp byte [edi], ' '
    je .token_split
    inc edi
    jmp .token_scan
.token_split:
    mov byte [edi], 0
    inc edi
.token_done:
    ret
.no_token:
    xor eax, eax
    ret

token_eq:
    push edi
    mov edi, ebx
.eq_loop:
    mov al, [edi]
    cmp al, [ecx]
    jne .eq_no
    test al, al
    je .eq_yes
    inc edi
    inc ecx
    jmp .eq_loop
.eq_yes:
    pop edi
    mov eax, 1
    ret
.eq_no:
    pop edi
    xor eax, eax
    ret

strlen:
    xor edx, edx
.strlen_loop:
    cmp byte [ecx + edx], 0
    je .strlen_done
    inc edx
    jmp .strlen_loop
.strlen_done:
    ret

print_cstr:
    push eax
    push ebx
    push edx
    call strlen
    mov eax, SYS_WRITE
    mov ebx, 1
    int 0x80
    pop edx
    pop ebx
    pop eax
    ret

msg_start:
    db "userspace shell started", 10, 0
prompt:
    db "$ ", 0
newline:
    db 10, 0
root_path:
    db "/", 0

cmd_help:
    db "help", 0
cmd_exit:
    db "exit", 0
cmd_ls:
    db "ls", 0
cmd_cat:
    db "cat", 0
cmd_mkdir:
    db "mkdir", 0
cmd_touch:
    db "touch", 0
cmd_rm:
    db "rm", 0
cmd_write:
    db "write", 0

msg_help:
    db "help", 10
    db "ls [path]", 10
    db "cat <path>", 10
    db "mkdir <path>", 10
    db "touch <path>", 10
    db "rm <path>", 10
    db "write <path> <text>", 10
    db "exit", 10, 0
msg_unknown:
    db "command not found", 10, 0
msg_missing:
    db "missing operand", 10, 0
msg_failed:
    db "failed", 10, 0
msg_ok:
    db "ok", 10, 0
user_shell_end:
