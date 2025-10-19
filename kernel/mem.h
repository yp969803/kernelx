#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../cpu/boot.h"

#define KERNEL_START 0xc0000000
#define KERNEL_MALLOC 0xd0000000
#define PAGE_FLAG_PRESENT (1<<0)
#define PAGE_FLAG_WRITE (1<<1)
#define PAGE_FLAG_OWNER (1 << 9)
#define PAGE_FLAG_GLOBAL (1 << 8)
#define PAGE_SIZE 0x1000
#define REC_PAGEDIR ((uint32_t*)0xFFFFF000)
#define REC_PAGETABLE(i) ((uint32_t*) (0xFFC00000 + ((i) << 12)))

extern uint32_t initial_page_dir[1024];

void* memmove(void* dest, const void* src, size_t n);
void memory_copy(void *src, void *dest, size_t nbytes);
void mem_set(void *dst, uint8_t val, size_t count);
void init_memory(uint32_t memHigh, uint32_t physicalAllocStart);
void invalidate(uint32_t vaddr);
void pmm_init(uint32_t memLow, uint32_t memHigh);
uint32_t pmmAllocPageFrame(void);
void memMapPage(uint32_t virtualAddr, uint32_t physAddr, uint32_t flags);
void pmmFreePageFrame(uint32_t paddr);