#include "kmalloc.h"
#include "mem.h"
#include "utils.h"

static uint32_t heapStart;
static uint32_t heapSize;
static uint32_t threshold;
static bool kmallocInitialized = false;

void changeHeapSize(int newSize){
    uint32_t oldPageTop = CEIL_DIV(heapSize, PAGE_SIZE);
    uint32_t newPageTop = CEIL_DIV(newSize, PAGE_SIZE);
    uint32_t diff = newPageTop - oldPageTop;

    for(uint32_t i=0; i< diff; i++) {
        uint32_t phys = pmmAllocPageFrame();
        memMapPage(KERNEL_MALLOC + oldPageTop * 0x1000 + i * 0x1000, phys, PAGE_FLAG_WRITE);
    }
    heapSize = newSize;
}

void kmallocInit(uint32_t initialHeapSize){
    heapStart = KERNEL_MALLOC;
    heapSize = 0;
    threshold = 0;
    kmallocInitialized = true;
    changeHeapSize(initialHeapSize);
}

