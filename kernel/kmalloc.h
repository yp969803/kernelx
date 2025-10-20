#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct KmallocHeader {
    uint32_t size; // In bytes
    bool free;
    struct KmallocHeader* next;
    struct KmallocHeader* prev;
} KmallocHeader;

void kmallocInit(uint32_t initialHeapSize);
void* kmalloc(uint32_t size);