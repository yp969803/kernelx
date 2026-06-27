#include "fat_vfs.h"
#include "../kernel/kmalloc.h"
#include "../kernel/mem.h"
#include "../kernel/utils.h"
#include "fat.h"
#include "vfs.h"
#include <stddef.h>

typedef struct {
    char path[128];
    char dir_name[11];
    uint16_t parent_cluster;
    uint16_t first_cluster;
} fat_vfs_file_t;

typedef struct {
    fat_directory_entry_t *entries;
    uint32_t count;
} fat_vfs_dir_t;

static int fat_vfs_file_read(vfs_file_t *file, uint8_t *buffer, uint32_t len);
static int fat_vfs_file_write(vfs_file_t *file, const uint8_t *buffer, uint32_t len);
static int fat_vfs_file_close(vfs_file_t *file);
static int fat_vfs_dir_read(vfs_file_t *file, vfs_dirent_t *entry);
static int fat_vfs_dir_close(vfs_file_t *file);

static const vfs_file_ops_t fat_file_ops = {
    fat_vfs_file_read,
    fat_vfs_file_write,
    NULL,
    fat_vfs_file_close,
};

static const vfs_file_ops_t fat_dir_ops = {
    NULL,
    NULL,
    fat_vfs_dir_read,
    fat_vfs_dir_close,
};

static int path_copy(char dst[128], const char *src)
{
    uint32_t i = 0;
    if (!src || src[0] != '/' || src[1] == '\0') {
        return ERR;
    }

    while (src[i] != '\0' && i < 127) {
        dst[i] = src[i];
        i++;
    }
    if (src[i] != '\0') {
        return ERR;
    }

    dst[i] = '\0';
    return OK;
}

static fat_directory_entry_t *lookup_entry(const char *path, char dir_name[11],
                                           uint16_t *parent_cluster)
{
    mem_set(dir_name, ' ', 11);
    *parent_cluster = ROOT_CLUSTER;
    return fat_lookup(path, dir_name, parent_cluster);
}

static int refresh_file_entry(vfs_file_t *file, fat_directory_entry_t **entry_out)
{
    fat_vfs_file_t *fat_file = file->fs_private;
    if (!fat_file || !entry_out) {
        return ERR;
    }

    fat_directory_entry_t *entry =
        lookup_entry(fat_file->path, fat_file->dir_name, &fat_file->parent_cluster);
    if (!entry || entry->attr == ATTR_DIRECTORY) {
        if (entry) {
            kfree(entry);
        }
        return ERR;
    }

    file->size              = entry->file_size;
    fat_file->first_cluster = entry->first_cluster_low;
    *entry_out              = entry;
    return OK;
}

static void format_fat_name(const fat_directory_entry_t *entry, char *buffer, uint32_t max)
{
    uint32_t pos      = 0;
    uint32_t name_end = 8;
    uint32_t ext_end  = 11;

    if (!entry || !buffer || max == 0) {
        return;
    }

    while (name_end > 0 && entry->name[name_end - 1] == ' ') {
        name_end--;
    }
    while (ext_end > 8 && entry->name[ext_end - 1] == ' ') {
        ext_end--;
    }

    for (uint32_t i = 0; i < name_end && pos < max - 1; i++) {
        buffer[pos++] = entry->name[i];
    }

    if (ext_end > 8 && pos < max - 1) {
        buffer[pos++] = '.';
        for (uint32_t i = 8; i < ext_end && pos < max - 1; i++) {
            buffer[pos++] = entry->name[i];
        }
    }

    if (entry->attr == ATTR_DIRECTORY && pos < max - 1) {
        buffer[pos++] = '/';
    }

    buffer[pos] = '\0';
}

static int open_dir(const char *path, uint32_t flags, vfs_file_t **out)
{
    if ((flags & O_ACCMODE) != O_RDONLY) {
        return ERR;
    }

    fat_directory_entry_t *entries = kmalloc(sizeof(fat_directory_entry_t) * 64);
    if (!entries) {
        return ERR;
    }

    uint32_t count = 0;
    if (fat_list_dir(path, entries, 64, &count) != OK) {
        kfree(entries);
        return ERR;
    }

    vfs_file_t *file = kmalloc(sizeof(vfs_file_t));
    if (!file) {
        kfree(entries);
        return ERR;
    }

    fat_vfs_dir_t *fat_dir = kmalloc(sizeof(fat_vfs_dir_t));
    if (!fat_dir) {
        kfree(file);
        kfree(entries);
        return ERR;
    }

    mem_set(file, 0, sizeof(vfs_file_t));
    mem_set(fat_dir, 0, sizeof(fat_vfs_dir_t));

    fat_dir->entries = entries;
    fat_dir->count   = count;

    file->mode       = S_IFDIR;
    file->size       = count;
    file->offset     = 0;
    file->flags      = flags;
    file->ref_count  = 1;
    file->fs_private = fat_dir;
    file->fops       = &fat_dir_ops;

    *out = file;
    return OK;
}

int fat_vfs_open(const char *path, uint32_t flags, vfs_file_t **out)
{
    if (!path || !out) {
        return ERR;
    }

    *out = NULL;

    if ((flags & O_DIRECTORY) && path[0] == '/' && path[1] == '\0') {
        return open_dir(path, flags, out);
    }

    fat_directory_entry_t *entry;
    char dir_name[11];
    uint16_t parent_cluster;

    entry = lookup_entry(path, dir_name, &parent_cluster);
    if (!entry) {
        if ((flags & O_DIRECTORY) || !(flags & O_CREAT) || mkfile_fat(path) != OK) {
            return ERR;
        }
        entry = lookup_entry(path, dir_name, &parent_cluster);
        if (!entry) {
            return ERR;
        }
    }

    if (entry->attr == ATTR_DIRECTORY) {
        if (flags & O_DIRECTORY) {
            kfree(entry);
            return open_dir(path, flags, out);
        }
        kfree(entry);
        return ERR;
    }

    if (flags & O_DIRECTORY) {
        kfree(entry);
        return ERR;
    }

    if ((flags & O_TRUNC) && ((flags & O_ACCMODE) != O_RDONLY)) {
        kfree(entry);
        if (fat_rm_entry(path) != OK || mkfile_fat(path) != OK) {
            return ERR;
        }
        entry = lookup_entry(path, dir_name, &parent_cluster);
        if (!entry) {
            return ERR;
        }
    }

    vfs_file_t *file = kmalloc(sizeof(vfs_file_t));
    if (!file) {
        kfree(entry);
        return ERR;
    }

    fat_vfs_file_t *fat_file = kmalloc(sizeof(fat_vfs_file_t));
    if (!fat_file) {
        kfree(file);
        kfree(entry);
        return ERR;
    }

    mem_set(file, 0, sizeof(vfs_file_t));
    mem_set(fat_file, 0, sizeof(fat_vfs_file_t));

    if (path_copy(fat_file->path, path) != OK) {
        kfree(fat_file);
        kfree(file);
        kfree(entry);
        return ERR;
    }

    mem_copy(dir_name, fat_file->dir_name, 11);
    fat_file->parent_cluster = parent_cluster;
    fat_file->first_cluster  = entry->first_cluster_low;

    file->mode       = S_IFREG;
    file->size       = entry->file_size;
    file->offset     = (flags & O_APPEND) ? entry->file_size : 0;
    file->flags      = flags;
    file->ref_count  = 1;
    file->fs_private = fat_file;
    file->fops       = &fat_file_ops;

    kfree(entry);
    *out = file;
    return OK;
}

static int fat_vfs_file_read(vfs_file_t *file, uint8_t *buffer, uint32_t len)
{
    fat_directory_entry_t *entry;
    if (refresh_file_entry(file, &entry) != OK) {
        return ERR;
    }

    if ((file->flags & O_ACCMODE) == O_WRONLY) {
        kfree(entry);
        return ERR;
    }

    if (file->offset >= entry->file_size) {
        kfree(entry);
        return 0;
    }

    uint32_t available = entry->file_size - file->offset;
    uint32_t to_read   = MIN(len, available);
    uint8_t *tmp       = kmalloc(entry->file_size);
    if (!tmp) {
        kfree(entry);
        return ERR;
    }

    if (entry->file_size > 0) {
        uint16_t cluster = entry->first_cluster_low;
        if (fat_read_file(&cluster, tmp, entry->file_size) != OK) {
            kfree(tmp);
            kfree(entry);
            return ERR;
        }
    }

    mem_copy(tmp + file->offset, buffer, to_read);
    file->offset += to_read;
    kfree(tmp);
    kfree(entry);
    return (int)to_read;
}

static int fat_vfs_file_write(vfs_file_t *file, const uint8_t *buffer, uint32_t len)
{
    if ((file->flags & O_ACCMODE) == O_RDONLY) {
        return ERR;
    }

    fat_directory_entry_t *entry;
    if (refresh_file_entry(file, &entry) != OK) {
        return ERR;
    }

    fat_vfs_file_t *fat_file = file->fs_private;
    uint32_t write_offset    = (file->flags & O_APPEND) ? entry->file_size : file->offset;
    uint32_t new_size        = MAX(entry->file_size, write_offset + len);
    uint8_t *tmp             = kmalloc(new_size);
    if (!tmp) {
        kfree(entry);
        return ERR;
    }

    mem_set(tmp, 0, new_size);
    if (entry->file_size > 0) {
        uint16_t read_cluster = entry->first_cluster_low;
        if (fat_read_file(&read_cluster, tmp, entry->file_size) != OK) {
            kfree(tmp);
            kfree(entry);
            return ERR;
        }
    }

    mem_copy((void *)buffer, tmp + write_offset, len);

    if (fat_rm_entry(fat_file->path) != OK || mkfile_fat(fat_file->path) != OK) {
        kfree(tmp);
        kfree(entry);
        return ERR;
    }

    fat_directory_entry_t *new_entry =
        lookup_entry(fat_file->path, fat_file->dir_name, &fat_file->parent_cluster);
    if (!new_entry) {
        kfree(tmp);
        kfree(entry);
        return ERR;
    }

    uint16_t cluster = new_entry->first_cluster_low;
    if (new_size > 0 && fat_write_file(&cluster, tmp, new_size) != OK) {
        kfree(new_entry);
        kfree(tmp);
        kfree(entry);
        return ERR;
    }

    new_entry->first_cluster_low = cluster;
    new_entry->file_size         = new_size;
    if (fat_update_dir_entry(fat_file->parent_cluster, fat_file->dir_name, new_entry) != OK) {
        kfree(new_entry);
        kfree(tmp);
        kfree(entry);
        return ERR;
    }

    fat_file->first_cluster = cluster;
    file->size              = new_size;
    file->offset            = write_offset + len;

    kfree(new_entry);
    kfree(tmp);
    kfree(entry);
    return (int)len;
}

static int fat_vfs_file_close(vfs_file_t *file)
{
    if (!file) {
        return ERR;
    }

    if (file->fs_private) {
        kfree(file->fs_private);
    }
    kfree(file);
    return OK;
}

static int fat_vfs_dir_read(vfs_file_t *file, vfs_dirent_t *entry)
{
    if (!file || !entry) {
        return ERR;
    }

    fat_vfs_dir_t *fat_dir = file->fs_private;
    if (!fat_dir || !fat_dir->entries) {
        return ERR;
    }

    if (file->offset >= fat_dir->count) {
        return 0;
    }

    fat_directory_entry_t *fat_entry = &fat_dir->entries[file->offset++];
    mem_set(entry, 0, sizeof(vfs_dirent_t));
    format_fat_name(fat_entry, entry->name, sizeof(entry->name));
    entry->mode = (fat_entry->attr == ATTR_DIRECTORY) ? S_IFDIR : S_IFREG;
    entry->size = fat_entry->file_size;

    return 1;
}

static int fat_vfs_dir_close(vfs_file_t *file)
{
    if (!file) {
        return ERR;
    }

    fat_vfs_dir_t *fat_dir = file->fs_private;
    if (fat_dir) {
        if (fat_dir->entries) {
            kfree(fat_dir->entries);
        }
        kfree(fat_dir);
    }

    kfree(file);
    return OK;
}

int fat_vfs_unlink(const char *path)
{
    return fat_rm_entry(path);
}

int fat_vfs_mkdir(const char *path)
{
    return mkdir_fat(path);
}
