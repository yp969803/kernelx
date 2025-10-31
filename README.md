# kernelx

kernelx is a simple lightweight 32bit x86 linux kernel, designed to run in small resources.

## Features

- Paging
- Dynamic memory allocation
- Keyboard
- VGA Text Mode
- Timer
- Muti-tasking
- Preemptive Scheduling

## Installations for ubuntu

Core Build Tools

```
sudo apt install build-essential gdb-multiarch make nasm clang-format
```

GRUB Tools

```
sudo apt install grub-pc-bin grub-common xorriso

```

Install Qemu

```
sudo apt install qemu qemu-system qemu-utils
```

Install Bochs
```
sudo apt install bochs bochs-x
```

## Run Locally

Running locally using qemu

```
make
```

Running locally using bochs
```
make run_bochs
```


Testing

```
make debug
```

```
make run_gdb
```