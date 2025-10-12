# $@ = target file
# $< = first dependency
# $^ = all dependencies

# detect all .o files based on their .c source
C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c)
HEADERS = $(wildcard kernel/*.h  drivers/*.h cpu/*.h)
OBJ_FILES = ${C_SOURCES:.c=.o cpu/isr_keyboard.o cpu/kernel_entry.o cpu/gdt.o} 

CC ?= x86_64-elf-gcc
LD ?= x86_64-elf-ld

# First rule is the one executed when no parameters are fed to the Makefile
all: run

run: iso
	qemu-system-i386 -cdrom kernelx.iso -m 512M -boot d

# only for debug
kernel.elf: ${OBJ_FILES}
	$(LD) -m elf_i386 -T link.ld -o $@ $^
	cp kernel.elf iso/boot/kernel.elf

iso: kernel.elf
	grub-mkrescue -o kernelx.iso iso/


debug: iso
	qemu-system-i386 -cdrom kernelx.iso -m 512M -boot d -S -s -d guest_errors,int

run_gdb: iso
	gdb-multiarch -ex "target remote localhost:1234" -ex "symbol-file kernel.elf"

%.o: %.c ${HEADERS}
	$(CC) -g -m32 -ffreestanding -fno-pie -fno-stack-protector -O1 -c $< -o $@ # -g for debugging

%.o: %.asm
	nasm $< -f elf -o $@

%.bin: %.asm
	nasm $< -f bin -o $@

%.dis: %.bin
	ndisasm -b 32 $< > $@

clean:
	$(RM) *.bin *.o *.dis *.elf
	$(RM) kernel/*.o
	$(RM) boot/*.o boot/*.bin
	$(RM) drivers/*.o
	$(RM) cpu/*.o
	$(RM) iso/boot/*.elf
	$(RM) kernelx.iso
