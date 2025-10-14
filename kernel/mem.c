#include "mem.h"
#include "../drivers/vga.h"

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

void init_memory(struct multiboot_info* mb_info) {
    for(uint32_t i = 0; i < mb_info->mmap_length; i+=sizeof(struct multiboot_mmap_entry)) {
        struct multiboot_mmap_entry* entry = (struct multiboot_mmap_entry*)(mb_info->mmap_addr + i);
        vga_print_hex(entry->addr_low);
        vga_print_string(" - ");
        vga_print_hex(entry->addr_low + entry->len_low);
        vga_print_string(" : ");

    }
}