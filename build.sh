#!/bin/bash
set -e

mkdir -p ./build
mkdir -p ./iso/boot/grub

# assemble boot
nasm -f elf32 ./src/boot.asm -o ./build/boot.o

# compile kernel
gcc -m32 -ffreestanding -c ./src/kernel.c -o build/kernel.o -O2 -Wall -Wextra

# link everything
ld -m elf_i386 -T ./src/linker.ld -o ./build/kernel.bin ./build/boot.o ./build/kernel.o

# iso structure
cp ./build/kernel.bin ./iso/boot/
cp ./src/grub.cfg ./iso/boot/grub/


grub-mkrescue -o os.iso iso
qemu-system-x86_64 -cdrom os.iso -serial stdio
# qemu-system-x86_64 -cdrom os.iso
