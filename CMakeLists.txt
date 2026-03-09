cmake_minimum_required(VERSION 3.16)
project(MyOS ASM C)

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
enable_language(ASM_NASM)

set(CMAKE_C_FLAGS "-m32 -ffreestanding -O2 -Wall -Wextra")
set(CMAKE_EXE_LINKER_FLAGS "-m elf_i386 -T ${CMAKE_SOURCE_DIR}/src/linker.ld --oformat binary")

set(BUILD_DIR   ${CMAKE_BINARY_DIR}/build)
set(ISO_DIR     ${CMAKE_BINARY_DIR}/iso)
set(ISO_BOOT    ${ISO_DIR}/boot)
set(ISO_GRUB    ${ISO_DIR}/boot/grub)

file(MAKE_DIRECTORY ${BUILD_DIR} ${ISO_BOOT} ${ISO_GRUB})

set(BOOT_ASM  ${CMAKE_SOURCE_DIR}/src/boot.asm)
set(KERNEL_C  ${CMAKE_SOURCE_DIR}/src/kernel.c)
set(LINKER_LD ${CMAKE_SOURCE_DIR}/src/linker.ld)
set(GRUB_CFG  ${CMAKE_SOURCE_DIR}/src/grub.cfg)

add_custom_command(
    OUTPUT  ${BUILD_DIR}/boot.o
    COMMAND nasm -f elf32 ${BOOT_ASM} -o ${BUILD_DIR}/boot.o
    DEPENDS ${BOOT_ASM}
)

add_custom_command(
    OUTPUT  ${BUILD_DIR}/kernel.o
    COMMAND gcc -m32 -ffreestanding -c ${KERNEL_C}
                -o ${BUILD_DIR}/kernel.o -O2 -Wall -Wextra
    DEPENDS ${KERNEL_C}
)

add_custom_command(
    OUTPUT  ${BUILD_DIR}/kernel.bin
    COMMAND ld -m elf_i386 -T ${LINKER_LD}
               -o ${BUILD_DIR}/kernel.bin
               ${BUILD_DIR}/boot.o ${BUILD_DIR}/kernel.o
    DEPENDS ${BUILD_DIR}/boot.o ${BUILD_DIR}/kernel.o ${LINKER_LD}
)

add_custom_command(
    OUTPUT  ${CMAKE_BINARY_DIR}/os.iso
    COMMAND ${CMAKE_COMMAND} -E copy ${BUILD_DIR}/kernel.bin ${ISO_BOOT}/
    COMMAND ${CMAKE_COMMAND} -E copy ${GRUB_CFG}             ${ISO_GRUB}/
    COMMAND grub-mkrescue -o ${CMAKE_BINARY_DIR}/os.iso ${ISO_DIR}
    DEPENDS ${BUILD_DIR}/kernel.bin ${GRUB_CFG}
)

add_custom_target(os ALL
    DEPENDS ${CMAKE_BINARY_DIR}/os.iso
)

add_custom_target(run
    COMMAND qemu-system-x86_64 -cdrom ${CMAKE_BINARY_DIR}/os.iso -serial stdio
    DEPENDS ${CMAKE_BINARY_DIR}/os.iso
)

add_custom_target(run-quiet
    COMMAND qemu-system-x86_64 -cdrom ${CMAKE_BINARY_DIR}/os.iso
    DEPENDS ${CMAKE_BINARY_DIR}/os.iso
)
