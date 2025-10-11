#include <stdint.h>

struct multiboot_info {
    uint32_t flags;            // Flags indicating which fields are valid
    uint32_t mem_lower;        // Amount of lower memory (below 1 MB) in KB
    uint32_t mem_upper;        // Amount of upper memory (above 1 MB) in KB
    uint32_t boot_device;      // Boot device (BIOS disk device)
    uint32_t cmdline;          // Pointer to the command line string
    uint32_t mods_count;       // Number of modules loaded
    uint32_t mods_addr;        // Pointer to the first module structure
    uint32_t syms[4];          // Symbol table information (deprecated)
    uint32_t mmap_length;      // Size of the memory map
    uint32_t mmap_addr;        // Pointer to the memory map
    uint32_t drives_length;    // Size of the BIOS drive information
    uint32_t drives_addr;      // Pointer to the BIOS drive information
    uint32_t config_table;     // Pointer to the ROM configuration table
    uint32_t boot_loader_name; // Pointer to the bootloader name string
    uint32_t apm_table;        // Pointer to the APM (Advanced Power Management) table
    uint32_t vbe_control_info; // VBE (VESA BIOS Extensions) control information
    uint32_t vbe_mode_info;    // VBE mode information
    uint16_t vbe_mode;         // VBE mode number
    uint16_t vbe_interface_seg; // VBE interface segment
    uint16_t vbe_interface_off; // VBE interface offset
    uint16_t vbe_interface_len; // VBE interface length
    uint64_t framebuffer_addr; // Physical address of the framebuffer
    uint32_t framebuffer_pitch; // Number of bytes per scanline
    uint32_t framebuffer_width; // Width of the framebuffer in pixels
    uint32_t framebuffer_height; // Height of the framebuffer in pixels
    uint8_t framebuffer_bpp;   // Bits per pixel
    uint8_t framebuffer_type;  // Type of framebuffer (indexed, RGB, etc.)
    uint8_t color_info[6];     // Color information for indexed framebuffers
} __attribute__((packed));;