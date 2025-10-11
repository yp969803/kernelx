OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)

SECTIONS
{
    . = 1M;

    .multiboot ALIGN(4) : {
       *(.multiboot)
     }

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(COMMON)
        *(.bss*)
    }
}

