#include "mem.h"
#include "../drivers/vga.h"
#include "utils.h"

static uint32_t pageFrameMin;   
static uint32_t pageFrameMax;
static uint32_t totalAlloc;

#define NUM_PAGES_DIRS 256
#define NUM_PAGE_FRAMES (0x100000000 / PAGE_SIZE / 8)

uint8_t physicalMemoryBitmap[NUM_PAGE_FRAMES / 8]; //Dynamically, bit array

static uint32_t pageDirs[NUM_PAGES_DIRS][1024] __attribute__((aligned(4096)));
static uint8_t pageDirUsed[NUM_PAGES_DIRS];


void* memmove(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;

    if (d < s) {
        for (size_t i = 0; i < n; i++)
            d[i] = s[i];
    } else {
        for (size_t i = n; i != 0; i--)
            d[i - 1] = s[i - 1];
    }

    return dest;
}


void memory_copy(void *src, void *dest, size_t nbytes) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    int i;
    for (i = 0; i < nbytes; i++) {
        *(d + i) = *(s + i);
    }
}

void mem_set(void *dst, uint8_t val, size_t count){
    uint8_t *temp = (uint8_t *)dst;
    for( ; count != 0; count--) *temp++ = val;
}

void init_memory(uint32_t memHigh, uint32_t physicalAllocStart) {
   initial_page_dir[0] = 0;
   invalidate(0);
   initial_page_dir[1023]=((uint32_t)initial_page_dir-KERNEL_START) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
   invalidate(0xFFFFF000);

   pmm_init(physicalAllocStart, memHigh);
}

void pmm_init(uint32_t memLow, uint32_t memHigh){
    pageFrameMin  = CEIL_DIV(memLow, PAGE_SIZE);
    pageFrameMax  = memHigh / PAGE_SIZE;
    totalAlloc = 0;
    mem_set(pageDirs, 0, sizeof(pageDirs));
    mem_set(pageDirUsed, 0, sizeof(pageDirUsed));

}

void invalidate(uint32_t vaddr){
    asm volatile("invlpg %0"::"m"(vaddr));
}