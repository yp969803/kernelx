#include "gdt.h"
#include "../kernel/mem.h"

extern void gdt_flush(uint32_t);
extern void tss_flush(void);

struct gdt_entry_struct gdt_entries[6];
struct gdt_ptr_struct   gdt_ptr;
struct tss_entry_struct tss_entry;

void initGdt(void){
    gdt_ptr.limit = (sizeof(struct gdt_entry_struct) * 6) - 1;
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

    writeTSS(5, 0x10, 0x0); // TSS Segment TSS=0x0028

    gdt_flush((uint32_t)&gdt_ptr);
    tss_flush();
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

void writeTSS(uint32_t num, uint16_t ss0, uint32_t esp0){
    uint32_t base = (uint32_t) &tss_entry;
    uint32_t limit = base + sizeof(tss_entry);

    setGdtEntry(num, base, limit, 0xE9, 0x00);
    mem_set(&tss_entry, 0, sizeof(tss_entry));

    tss_entry.ss0 = ss0;
    tss_entry.esp0 = esp0;

    tss_entry.cs = 0x08 | 0x3;
    tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs = 0x10 | 0x3;
}

