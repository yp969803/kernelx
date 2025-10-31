#pragma once

#include <stdbool.h>
#include <stdint.h>

#define HEAP_END 0xDFFFFFFF

#define ALIGNMENT 8
#define ALIGN(x) (((x) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

typedef struct KmallocHeader {
    uint32_t size; // In bytes
    bool free;
    struct KmallocHeader *next;
    struct KmallocHeader *prev;
} KmallocHeader;

void kmallocInit(uint32_t initialHeapSize);
void *kmalloc(uint32_t size);
void kfree(void *ptr);
void *krealloc(void *ptr, uint32_t size);