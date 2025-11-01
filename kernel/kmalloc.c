#include "kmalloc.h"
#include "mem.h"
#include "mutex.h"
#include "utils.h"

static uint32_t heapStart;
static uint32_t heapSize;
static bool kmallocInitialized = false;

static spinlock heap_lock;

void increaseHeapSize(int newSize)
{
    uint32_t oldPageTop = CEIL_DIV(heapSize, PAGE_SIZE);
    uint32_t newPageTop = CEIL_DIV(newSize, PAGE_SIZE);
    uint32_t diff       = newPageTop - oldPageTop;

    for (uint32_t i = 0; i < diff; i++) {
        uint32_t phys = pmmAllocPageFrame();
        memMapPage(HEAP_START + oldPageTop * 0x1000 + i * 0x1000, phys, PAGE_FLAG_WRITE);
    }
    heapSize = newSize;
}

void decreaseHeapSize(int newSize)
{
    uint32_t oldPageTop = CEIL_DIV(heapSize, PAGE_SIZE);
    uint32_t newPageTop = CEIL_DIV(newSize, PAGE_SIZE);
    uint32_t diff       = oldPageTop - newPageTop;

    for (uint32_t i = 0; i < diff; i++) {
        memUnMapPage(HEAP_START + newPageTop * 0x1000 + i * 0x1000);
    }
    heapSize = newSize;
}

void kmallocInit(uint32_t initialHeapSize)
{
    heapStart          = HEAP_START;
    heapSize           = 0;
    kmallocInitialized = true;
    increaseHeapSize(initialHeapSize);
    KmallocHeader *header = (KmallocHeader *)heapStart;
    header->size          = heapSize - sizeof(KmallocHeader);
    header->free          = true;
    header->next          = NULL;
    header->prev          = NULL;

    spinlock_init(&heap_lock);
}

void *kmalloc(uint32_t size)
{
    if (size == 0 || !kmallocInitialized) {
        return NULL;
    }

    spinlock_lock(&heap_lock);

    size = ALIGN(size);

    KmallocHeader *current = (KmallocHeader *)heapStart;
    while (current != NULL) {
        if (current->free && current->size >= size) {
            if (current->size >= size + sizeof(KmallocHeader) + ALIGNMENT) {
                KmallocHeader *newHeader =
                    (KmallocHeader *)((uint32_t)current + sizeof(KmallocHeader) + size);
                newHeader->size = current->size - size - sizeof(KmallocHeader);
                newHeader->free = true;
                newHeader->next = current->next;
                newHeader->prev = current;

                if (current->next != NULL) {
                    current->next->prev = newHeader;
                }

                current->next = newHeader;
                current->size = size;
            }
            current->free = false;
            spinlock_unlock(&heap_lock);
            return (void *)((uint32_t)current + sizeof(KmallocHeader));
        }
        current = current->next;
    }

    while (heapSize < HEAP_END - HEAP_START) {
        uint32_t oldHeapSize = heapSize;
        increaseHeapSize(heapSize + PAGE_SIZE);
        KmallocHeader *newHeader = (KmallocHeader *)(heapStart + oldHeapSize);
        newHeader->size          = PAGE_SIZE - sizeof(KmallocHeader);
        newHeader->free          = true;
        newHeader->next          = NULL;
        newHeader->prev          = NULL;

        KmallocHeader *last = (KmallocHeader *)heapStart;
        while (last->next != NULL) {
            last = last->next;
        }
        if (last->free) {
            current = last;
            last->size += sizeof(KmallocHeader) + newHeader->size;
            if (last->prev && last->prev->free) {
                last->prev->size += sizeof(KmallocHeader) + last->size;
                last->prev->next = NULL;
                current          = last->prev;
            }
        } else {
            current         = newHeader;
            last->next      = newHeader;
            newHeader->prev = last;
        }

        if (current->free && current->size >= size) {
            if (current->size >= size + sizeof(KmallocHeader) + ALIGNMENT) {
                KmallocHeader *splitHeader =
                    (KmallocHeader *)((uint32_t)current + sizeof(KmallocHeader) + size);
                splitHeader->size = current->size - size - sizeof(KmallocHeader);
                splitHeader->free = true;
                splitHeader->next = current->next;
                splitHeader->prev = current;

                if (current->next != NULL) {
                    current->next->prev = splitHeader;
                }

                current->next = splitHeader;
                current->size = size;
            }
            current->free = false;
            spinlock_unlock(&heap_lock);
            return (void *)((uint32_t)current + sizeof(KmallocHeader));
        }
    }
    spinlock_unlock(&heap_lock);
    return NULL;
}

void kfree(void *ptr)
{
    if (ptr == NULL || !kmallocInitialized) {
        return;
    }

    spinlock_lock(&heap_lock);

    KmallocHeader *header = (KmallocHeader *)((uint32_t)ptr - sizeof(KmallocHeader));
    header->free          = true;

    // Coalesce with next block if free
    if (header->next != NULL && header->next->free) {
        header->size += sizeof(KmallocHeader) + header->next->size;
        header->next = header->next->next;
        if (header->next != NULL) {
            header->next->prev = header;
        }
    }

    // Coalesce with previous block if free
    if (header->prev != NULL && header->prev->free) {
        header->prev->size += sizeof(KmallocHeader) + header->size;
        header->prev->next = header->next;
        if (header->next != NULL) {
            header->next->prev = header->prev;
        }
    }
    spinlock_unlock(&heap_lock);
}

void *krealloc(void *ptr, uint32_t size)
{
    if (ptr == NULL) {
        return kmalloc(size);
    }

    if (size == 0) {
        kfree(ptr);
        return NULL;
    }

    KmallocHeader *header = (KmallocHeader *)((uint32_t)ptr - sizeof(KmallocHeader));
    if (header->size >= size) {
        return ptr; // Current block is sufficient
    }

    void *newPtr = kmalloc(size);
    if (newPtr != NULL) {
        memory_copy(ptr, newPtr, size);
        kfree(ptr);
    }
    return newPtr;
}