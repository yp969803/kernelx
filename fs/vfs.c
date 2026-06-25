#include "vfs.h"
#include "../kernel/kmalloc.h"
#include "../kernel/mem.h"
#include "../kernel/utils.h"
#include <stddef.h>

int fat_vfs_open(const char *path, uint32_t flags, vfs_file_t **out);
int fat_vfs_unlink(const char *path);
int fat_vfs_mkdir(const char *path);

void vfs_init(void) {}

int vfs_open(const char *path, uint32_t flags, vfs_file_t **out)
{
    if (!path || !out) {
        return ERR;
    }

    *out = NULL;
    return fat_vfs_open(path, flags, out);
}

int vfs_read(vfs_file_t *file, uint8_t *buffer, uint32_t len)
{
    if (!file || !buffer) {
        return ERR;
    }
    if (len == 0) {
        return 0;
    }
    if (!file->fops || !file->fops->read) {
        return ERR;
    }

    return file->fops->read(file, buffer, len);
}

int vfs_write(vfs_file_t *file, const uint8_t *buffer, uint32_t len)
{
    if (!file || !buffer) {
        return ERR;
    }
    if (len == 0) {
        return 0;
    }
    if (!file->fops || !file->fops->write) {
        return ERR;
    }

    return file->fops->write(file, buffer, len);
}

int vfs_close(vfs_file_t *file)
{
    if (!file) {
        return ERR;
    }

    vfs_file_release(file);
    return OK;
}

int vfs_lseek(vfs_file_t *file, int32_t offset, uint32_t whence)
{
    if (!file) {
        return ERR;
    }

    int32_t base;
    if (whence == SEEK_SET) {
        base = 0;
    } else if (whence == SEEK_CUR) {
        base = (int32_t)file->offset;
    } else if (whence == SEEK_END) {
        base = (int32_t)file->size;
    } else {
        return ERR;
    }

    int32_t new_offset = base + offset;
    if (new_offset < 0) {
        return ERR;
    }

    file->offset = (uint32_t)new_offset;
    return new_offset;
}

int vfs_unlink(const char *path)
{
    if (!path) {
        return ERR;
    }

    return fat_vfs_unlink(path);
}

int vfs_mkdir(const char *path)
{
    if (!path) {
        return ERR;
    }

    return fat_vfs_mkdir(path);
}

vfs_file_t *vfs_file_retain(vfs_file_t *file)
{
    if (file) {
        file->ref_count++;
    }
    return file;
}

void vfs_file_release(vfs_file_t *file)
{
    if (!file || file->ref_count == 0) {
        return;
    }

    file->ref_count--;
    if (file->ref_count > 0) {
        return;
    }

    if (file->fops && file->fops->close) {
        file->fops->close(file);
        return;
    }

    kfree(file);
}
