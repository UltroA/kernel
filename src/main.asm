org 0x7C00
bits 16

%define ENDL 0X0D, 0X0A

start:
    jmp main

; Outputs string on screen
; params:
;   ds:si - points to string
puts:
    push si
    push ax
    push bx

.loop:
    lodsb; load next char to al register
    or al, al; check if al is null
    jz .done

    mov ah, 0x0e
    xor bh, bh
    int 0x10

    jmp .loop

.done:
    pop bx
    pop ax
    pop si
    ret

main:
    ; data segments
    xor ax, ax
    mov ds, ax
    mov es, ax

    ; setup stack
    mov ss, ax
    mov sp, 0x7C00

    ; PRINT MSG
    mov si, msg_fuck
    call puts

    hlt

.helt:
    jmp .helt

msg_fuck: db 'Test output', ENDL, 0

times 510-($-$$) db 0
dw 0AA55h
