#pragma once

#include <stdbool.h>
#include <stdint.h>

#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR 0x0002
#define O_ACCMODE 0x0003
#define O_CREAT 0x0040
#define O_TRUNC 0x0200
#define O_APPEND 0x0400
#define O_DIRECTORY 0x10000

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define S_IFREG 0100000
#define S_IFDIR 0040000

typedef struct vfs_file vfs_file_t;

typedef struct {
    char name[256];
    uint32_t mode;
    uint32_t size;
} vfs_dirent_t;

typedef struct vfs_file_ops {
    int (*read)(vfs_file_t *file, uint8_t *buffer, uint32_t len);
    int (*write)(vfs_file_t *file, const uint8_t *buffer, uint32_t len);
    int (*readdir)(vfs_file_t *file, vfs_dirent_t *entry);
    int (*close)(vfs_file_t *file);
} vfs_file_ops_t;

struct vfs_file {
    uint32_t mode;
    uint32_t size;
    uint32_t offset;
    uint32_t flags;
    uint32_t ref_count;
    void *fs_private;
    const vfs_file_ops_t *fops;
};

void vfs_init(void);
int vfs_open(const char *path, uint32_t flags, vfs_file_t **out);
int vfs_read(vfs_file_t *file, uint8_t *buffer, uint32_t len);
int vfs_write(vfs_file_t *file, const uint8_t *buffer, uint32_t len);
int vfs_close(vfs_file_t *file);
int vfs_lseek(vfs_file_t *file, int32_t offset, uint32_t whence);
int vfs_readdir(vfs_file_t *file, vfs_dirent_t *entry);
int vfs_unlink(const char *path);
int vfs_mkdir(const char *path);
vfs_file_t *vfs_file_retain(vfs_file_t *file);
void vfs_file_release(vfs_file_t *file);
