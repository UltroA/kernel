org 0x7C00
bits 16

%define ENDL 0X0D, 0X0A

; ---FAT12 headers---
jmp short start
nop

bdb_oem:                    db 'rOS     '           ; 8 bytes
bdb_bytes_per_sector:       dw 512
bdb_sectors_per_cluster:    db 1
bdb_reserved_sectors:       dw 1
bdb_fat_count:              db 2
bdb_dir_entries_count:      dw 0E0h
bdb_total_sectors:          dw 2880                 ; 2880 * 512 = 1.44MB
bdb_media_descriptor_type:  db 0F0h                 ; F0 = 3.5" floppy disk
bdb_sectors_per_fat:        dw 9                    ; 9 sectors/fat
bdb_sectors_per_track:      dw 18
bdb_heads:                  dw 2
bdb_hidden_sectors:         dd 0
bdb_large_sector_count:     dd 0

; extended boot record
ebr_drive_number:           db 0                    ; 0x00 floppy, 0x80 hdd, useless
                            db 0                    ; reserved
ebr_signature:              db 29h
ebr_volume_id:              db 12h, 34h, 56h, 78h   ; serial number, value doesn't matter
ebr_volume_label:           db 'rOS        '        ; 11 bytes, padded with spaces
ebr_system_id:              db 'FAT12   '           ; 8 bytes


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

    ; read info from floppy
    mov [ebr_drive_number], dl
    
    mov ax, 1
    mov cl, 1
    mov bx, 0x7E00
    call diskR

    ; PRINT MSG
    mov si, msg_fuck
    call puts

    cli
    hlt


; ---Error handling section---
floppy_error: 
    mov si, msg_failed2read
    call puts
    jmp wait4key2reboot

wait4key2reboot:
    mov ah, 0
    int 16h
    jmp 0FFFFh:0 

.halt:
    cli; disable input 
    hlt

; --- Disk work---

; params:
;   ax - lbs address
; ret
; cx[0-5] - sector number
; cx[6-15] - cylinder
; dh - head
lba2chs:
    push ax
    push dx

    xor dx, dx
    div word[bdb_sectors_per_track]; ax = LBA/SPT

    inc dx 
    mov cx, dx 

    xor dx, dx
    div word[bdb_heads]

    mov dh, dl
    mov ch, al
    shl ah, 6
    or cl, ah
    
    pop ax
    mov dl, al
    pop ax
    ret

; Read sector from disk
; params: 
;   ax - LBA address
;   cl - n of sectors to read
;   dl - drive number
;   es:bx - location to write memory to
diskR:
    push ax
    push bx
    push cx
    push dx
    push di

    push cx
    call lba2chs
    pop ax

    mov ah, 02h
    mov di, 3; number of attempts to read memory (osDev docs recommends)

    .retry:
        pusha; saves all registers
        stc; set carryFlag
        int 13h; if not carryFlag = pass
        jnc .retry_end
        
        ; if failed to read information from floppy 
        popa
        call disk_reset

        dec di
        test di, di
        jnz .retry

    .failed2read:
        jmp floppy_error
    .retry_end:
        popa

        pop di
        pop dx
        pop cx
        pop bx
        pop ax
        ret

; ---Disk reset---
; params:
;   dl - drive number
disk_reset:
    pusha
    mov ah, 0
    stc
    int 13h
    jc floppy_error
    popa
    ret

; ---Basic kernel messages--- 
msg_fuck:           db 'Test output', ENDL, 0
msg_failed2read:    db '!!!Failed to read informtaion from floppy drive!!!', ENDL, 0

times 510-($-$$) db 0
dw 0AA55h
