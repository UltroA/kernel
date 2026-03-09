BITS 64

section .multiboot
align 8
mb_header:
    dd 0xE85250D6
    dd 0
    dd mb_header_end - mb_header
    dd -(0xE85250D6 + 0 + (mb_header_end - mb_header))

    dw 0
    dw 0
    dd 8

mb_header_end:

section .text
global _start
extern kernel_main

_start:
    cli
    call kernel_main

hang:
    hlt
    jmp hang
