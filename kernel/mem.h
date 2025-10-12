#pragma once

#include <stddef.h>
#include <stdint.h>

void* memmove(void* dest, const void* src, size_t n);
void memory_copy(void *src, void *dest, size_t nbytes);