 global gdt_flush 
  
  gdt_flush:
    mov eax, [esp + 4] ; get pointer to gdt_descriptor
    lgdt [eax]         ; load the GDT

    mov eax, 0x10     ; load the data segment selector (your DATA_SEG from GDT)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush  ; jump to code segment selector 

.flush
    ret

global tss_flush

  tss_flush:
    mov ax, 0x2B     ; TSS segment selector (your TSS_SEG from GDT)
    ltr ax           ; load the TSS
    ret 