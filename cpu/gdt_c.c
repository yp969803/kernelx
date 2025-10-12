#include "gdt.h"

extern void gdt_flush(uint32_t);

struct gdt_entry_struct gdt_entries[5];
struct gdt_ptr_struct   gdt_ptr;

void initGdt(void){
    gdt_ptr.limit = (sizeof(struct gdt_entry_struct) * 5) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    // NULL Segment
    setGdtEntry(0, 0, 0, 0, 0);
    
    // Kernel Code Segment CS=0x0008
    setGdtEntry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    
    // Kernel Data Segment DS=0x0010
    setGdtEntry(2, 0, 0xFFFFFFFF, 0x92, 0xCF); 

    // User Mode Code Segment CS=0x001B
    setGdtEntry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    
    // User Mode Data Segment DS=0x0020
    setGdtEntry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); 

    gdt_flush((uint32_t)&gdt_ptr);
}

void setGdtEntry(uint32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags){
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].flags       = (limit >> 16) & 0x0F;

    gdt_entries[num].flags      |= flags & 0xF0;
    gdt_entries[num].access      = access;
}