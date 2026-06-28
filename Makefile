# $@ = target file
# $< = first dependency
# $^ = all dependencies

# detect all .o files based on their .c source
C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c stdlib/*.c fs/*.c)
HEADERS = $(wildcard kernel/*.h  drivers/*.h cpu/*.h stdlib/*.h fs/*.h)
OBJ_FILES = ${C_SOURCES:.c=.o cpu/isr_keyboard.o cpu/kernel_entry.o cpu/gdt.o cpu/exception_isr.o cpu/isr_pit.o cpu/isr_ata.o cpu/switch_task.o cpu/syscall_isr.o cpu/user_shell.o}
DISK_IMG = disk.img
QEMU = qemu-system-i386
QEMU_FLAGS = -cdrom kernelx.iso -drive file=$(DISK_IMG),format=raw,if=ide,index=0,media=disk -m 512M -boot d

CC ?= x86_64-elf-gcc
LD ?= x86_64-elf-ld

.PHONY: all run debug run_gdb run_bochs clean format disk_img reset_disk_img

# First rule is the one executed when no parameters are fed to the Makefile
all: run

run: iso $(DISK_IMG)
	$(QEMU) $(QEMU_FLAGS)

# only for debug
kernel.elf: ${OBJ_FILES}
	$(LD) -m elf_i386 -T link.ld -o $@ $^
	cp kernel.elf iso/boot/kernel.elf

iso: kernel.elf
	grub-mkrescue -o kernelx.iso iso/


debug: iso $(DISK_IMG)
	$(QEMU) $(QEMU_FLAGS) -S -s -d guest_errors,int

run_gdb: iso
	gdb-multiarch -ex "target remote localhost:1234" -ex "symbol-file kernel.elf"

run_bochs: iso
	bochs -f bochsrc

%.o: %.c ${HEADERS}
	$(CC) -g -m32 -ffreestanding -fno-pie -fno-stack-protector -O0 -c $< -o $@ # -g for debugging

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
	$(RM) fs/*.o
	$(RM) stdlib/*.o
	$(RM) iso/boot/*.elf
	$(RM) kernelx.iso

format:
	find . -regex '.*\.\(c\|h\)$$' -exec clang-format -i {} \;


$(DISK_IMG):
	qemu-img create -f raw $(DISK_IMG) 64M

disk_img: $(DISK_IMG)

reset_disk_img:
	qemu-img create -f raw $(DISK_IMG) 64M
