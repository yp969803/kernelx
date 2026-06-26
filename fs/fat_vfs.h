#pragma once

#include "vfs.h"

int fat_vfs_open(const char *path, uint32_t flags, vfs_file_t **out);
int fat_vfs_unlink(const char *path);
int fat_vfs_mkdir(const char *path);
