#include "syscall.h"
#include "../drivers/keyboard.h"
#include "../drivers/timer.h"
#include "../drivers/vga.h"
#include "../fs/vfs.h"
#include "../kernel/kmalloc.h"
#include "../kernel/utils.h"
#include "../stdlib/stdio.h"
#include "mem.h"
#include "process.h"
#include "task.h"

#define SYSCALL_PATH_MAX 127
#define SYSCALL_IO_CHUNK 512

static int copy_user_string(uint32_t user_src, char *kernel_dst, uint32_t max_len)
{
    if (!current_task_TCB || !current_task_TCB->process || !user_src || !kernel_dst ||
        max_len == 0) {
        return ERR;
    }

    for (uint32_t i = 0; i < max_len; i++) {
        if (copy_from_user(current_task_TCB->process->page_dir, &kernel_dst[i], user_src + i, 1) !=
            OK) {
            return ERR;
        }
        if (kernel_dst[i] == '\0') {
            return OK;
        }
    }

    kernel_dst[max_len - 1] = '\0';
    return ERR;
}

int sys_write(uint32_t fd, uint32_t buf, uint32_t len, uint32_t unused3, uint32_t unused4,
              uint32_t unused5)
{
    (void)unused3;
    (void)unused4;
    (void)unused5;

    if (len == 0) {
        return 0;
    }

    if (!current_task_TCB || !buf) {
        return ERR;
    }

    file_descriptor_t *desc = process_get_fd(current_task_TCB->process, fd);
    if (!desc) {
        return ERR;
    }

    if (desc->type != FD_STDOUT && desc->type != FD_STDERR) {
        if (!desc->file) {
            return ERR;
        }

        uint8_t *buffer = kmalloc(SYSCALL_IO_CHUNK);
        if (!buffer) {
            return ERR;
        }

        uint32_t total = 0;
        while (total < len) {
            uint32_t chunk = len - total;
            if (chunk > SYSCALL_IO_CHUNK) {
                chunk = SYSCALL_IO_CHUNK;
            }
            if (copy_from_user(current_task_TCB->process->page_dir, buffer, buf + total, chunk) !=
                OK) {
                kfree(buffer);
                return ERR;
            }

            int written = vfs_write(desc->file, buffer, chunk);
            if (written < 0) {
                kfree(buffer);
                return ERR;
            }
            total += (uint32_t)written;
            if ((uint32_t)written != chunk) {
                break;
            }
        }

        kfree(buffer);
        return (int)total;
    }

    for (uint32_t i = 0; i < len; i++) {
        char ch;
        if (copy_from_user(current_task_TCB->process->page_dir, &ch, buf + i, 1) != OK) {
            return ERR;
        }
        kprintf("%c", ch);
    }

    return (int)len;
}

int sys_read(uint32_t fd, uint32_t buf, uint32_t len, uint32_t unused3, uint32_t unused4,
             uint32_t unused5)
{
    (void)unused3;
    (void)unused4;
    (void)unused5;

    if (len == 0) {
        return 0;
    }
    if (!current_task_TCB || !current_task_TCB->process || !buf) {
        return ERR;
    }

    file_descriptor_t *desc = process_get_fd(current_task_TCB->process, fd);
    if (!desc) {
        return ERR;
    }

    if (desc->type != FD_STDIN) {
        if (!desc->file) {
            return ERR;
        }

        if (memValidateUserBuffer(current_task_TCB->process->page_dir, buf, len, true) != OK) {
            return ERR;
        }

        uint8_t *buffer = kmalloc(SYSCALL_IO_CHUNK);
        if (!buffer) {
            return ERR;
        }

        uint32_t total = 0;
        while (total < len) {
            uint32_t chunk = len - total;
            if (chunk > SYSCALL_IO_CHUNK) {
                chunk = SYSCALL_IO_CHUNK;
            }

            int read = vfs_read(desc->file, buffer, chunk);
            if (read < 0) {
                kfree(buffer);
                return ERR;
            }
            if (read == 0) {
                break;
            }
            if (copy_to_user(current_task_TCB->process->page_dir, buf + total, buffer,
                             (uint32_t)read) != OK) {
                kfree(buffer);
                return ERR;
            }

            total += (uint32_t)read;
            if ((uint32_t)read != chunk) {
                break;
            }
        }

        kfree(buffer);
        return (int)total;
    }

    if (memValidateUserBuffer(current_task_TCB->process->page_dir, buf, len, true) != OK) {
        return ERR;
    }

    uint32_t count = 0;
    task_scheduler_disable();
    set_interrupt();

    while (count < len) {
        char ch = (char)keyboard_getchar();

        if (ch == '\b') {
            if (count > 0) {
                count--;
                vga_remove_char();
            }
            continue;
        }

        if (copy_to_user(current_task_TCB->process->page_dir, buf + count, &ch, 1) != OK) {
            clear_interrupt();
            task_scheduler_enable();
            return ERR;
        }

        count++;
        if (ch == '\n') {
            vga_put_char('\n');
            break;
        }

        vga_put_char((uint8_t)ch);
    }

    clear_interrupt();
    task_scheduler_enable();
    return (int)count;
}

int sys_exit(uint32_t code, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4,
             uint32_t unused5)
{
    (void)code;
    if (current_task_TCB) {
        current_task_TCB->state = TASK_ZOOMBIE;
        task_pick_next();
    }
    return 0;
}

int sys_sleep(uint32_t milliseconds, uint32_t unused1, uint32_t unused2, uint32_t unused3,
              uint32_t unused4, uint32_t unused5)
{
    if (sleep_current_task(milliseconds) != OK) {
        return ERR;
    }

    task_pick_next();
    return OK;
}

int sys_open(uint32_t path, uint32_t flags, uint32_t unused2, uint32_t unused3, uint32_t unused4,
             uint32_t unused5)
{
    (void)unused2;
    (void)unused3;
    (void)unused4;
    (void)unused5;

    char kernel_path[SYSCALL_PATH_MAX + 1];
    if (copy_user_string(path, kernel_path, sizeof(kernel_path)) != OK) {
        return ERR;
    }

    vfs_file_t *file;
    if (vfs_open(kernel_path, flags, &file) != OK) {
        return ERR;
    }

    int fd = process_alloc_fd(current_task_TCB->process, file);
    vfs_file_release(file);
    return fd;
}

int sys_close(uint32_t fd, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4,
              uint32_t unused5)
{
    (void)unused1;
    (void)unused2;
    (void)unused3;
    (void)unused4;
    (void)unused5;

    return process_close_fd(current_task_TCB->process, fd);
}

int sys_lseek(uint32_t fd, uint32_t offset, uint32_t whence, uint32_t unused3, uint32_t unused4,
              uint32_t unused5)
{
    (void)unused3;
    (void)unused4;
    (void)unused5;

    vfs_file_t *file = process_get_file(current_task_TCB->process, fd);
    if (!file) {
        return ERR;
    }

    return vfs_lseek(file, (int32_t)offset, whence);
}

int sys_unlink(uint32_t path, uint32_t unused1, uint32_t unused2, uint32_t unused3,
               uint32_t unused4, uint32_t unused5)
{
    (void)unused1;
    (void)unused2;
    (void)unused3;
    (void)unused4;
    (void)unused5;

    char kernel_path[SYSCALL_PATH_MAX + 1];
    if (copy_user_string(path, kernel_path, sizeof(kernel_path)) != OK) {
        return ERR;
    }

    return vfs_unlink(kernel_path);
}

int sys_mkdir(uint32_t path, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4,
              uint32_t unused5)
{
    (void)unused1;
    (void)unused2;
    (void)unused3;
    (void)unused4;
    (void)unused5;

    char kernel_path[SYSCALL_PATH_MAX + 1];
    if (copy_user_string(path, kernel_path, sizeof(kernel_path)) != OK) {
        return ERR;
    }

    return vfs_mkdir(kernel_path);
}

int sys_getdents(uint32_t fd, uint32_t buf, uint32_t len, uint32_t unused3, uint32_t unused4,
                 uint32_t unused5)
{
    (void)unused3;
    (void)unused4;
    (void)unused5;

    if (!current_task_TCB || !current_task_TCB->process || !buf || len < sizeof(vfs_dirent_t)) {
        return ERR;
    }

    if (memValidateUserBuffer(current_task_TCB->process->page_dir, buf, len, true) != OK) {
        return ERR;
    }

    vfs_file_t *file = process_get_file(current_task_TCB->process, fd);
    if (!file) {
        return ERR;
    }

    uint32_t written = 0;
    while (written + sizeof(vfs_dirent_t) <= len) {
        vfs_dirent_t entry;
        int result = vfs_readdir(file, &entry);
        if (result < 0) {
            return written ? (int)written : ERR;
        }
        if (result == 0) {
            break;
        }

        if (copy_to_user(current_task_TCB->process->page_dir, buf + written, &entry,
                         sizeof(vfs_dirent_t)) != OK) {
            return written ? (int)written : ERR;
        }
        written += sizeof(vfs_dirent_t);
    }

    return (int)written;
}

syscall_t syscall_table[] = {
    (syscall_t)sys_write, (syscall_t)sys_exit,     (syscall_t)sys_sleep, (syscall_t)sys_read,
    (syscall_t)sys_open,  (syscall_t)sys_close,    (syscall_t)sys_lseek, (syscall_t)sys_unlink,
    (syscall_t)sys_mkdir, (syscall_t)sys_getdents,
};

int syscall_handler_c(pt_regs *regs)
{
    uint32_t num  = regs->eax; // syscall number
    uint32_t arg1 = regs->ebx;
    uint32_t arg2 = regs->ecx;
    uint32_t arg3 = regs->edx;
    uint32_t arg4 = regs->esi;
    uint32_t arg5 = regs->edi;
    uint32_t arg6 = regs->ebp;

    if (num >= sizeof(syscall_table) / sizeof(syscall_t)) {
        regs->eax = -1; // invalid syscall
        return -1;
    }

    syscall_t func = syscall_table[num];
    int result = func(arg1, arg2, arg3, arg4, arg5, arg6);
    regs->eax  = result;
    if (current_task_TCB && current_task_TCB->state == TASK_RUNNING) {
        next_task_TCB = current_task_TCB;
    }
    return result;
}
