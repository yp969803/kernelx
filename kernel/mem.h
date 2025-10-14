#pragma once

#include <stddef.h>
#include <stdint.h>
#include "../cpu/boot.h"

void* memmove(void* dest, const void* src, size_t n);
void memory_copy(void *src, void *dest, size_t nbytes);
void mem_set(void *dst, uint8_t val, size_t count);
void init_memory(struct multiboot_info* mb_info);