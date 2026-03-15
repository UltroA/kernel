org 0x7C00
bits 16

main:
    hlt

.helt:
    jmp .helt

times 510-($-$$) db 0
dw 0AA55h
