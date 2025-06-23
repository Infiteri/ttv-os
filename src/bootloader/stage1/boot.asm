org 0x7C00
bits 16

%define ENDL 0x0D, 0x0A

jmp short start
nop

; --- BIOS Parameter Block (BPB) and Extended Boot Record (EBR) ---
bdb_oem:                    db 'MSWIN4.1'
bdb_bytes_per_sector:       dw 512
bdb_sectors_per_cluster:    db 1
bdb_reserved_sectors:       dw 1
bdb_fat_count:              db 2
bdb_dir_entries_count:      dw 0E0h
bdb_total_sectors:          dw 2880
bdb_media_descriptor_type:  db 0F0h
bdb_sectors_per_fat:        dw 9
bdb_sectors_per_track:      dw 18
bdb_heads:                  dw 2
bdb_hidden_sectors:         dd 0
bdb_large_sector_count:     dd 0
ebr_drive_number:           db 0
                            db 0                ; reserved
ebr_signature:              db 29h
ebr_volume_id:              db 12h, 34h, 56h, 78h
ebr_volume_label:           db 'TTV      OS'
ebr_system_id:              db 'FAT12   '

; --- Start of code ---
start:
    mov ax, 0
    mov ds, ax
    mov es, ax

    mov ss, ax
    mov sp, 0x7C00

    push es
    push word .after
    retf 

.after:

    mov [ebr_drive_number], dl     ; Save BIOS boot drive number

    mov si, loading_msg
    call print

    push es

    mov ah, 08h
    int 13h
    jc floppy_error

    pop es

    and cl, 0x3F
    xor ch, ch
    mov [bdb_sectors_per_track], cx

    inc dh
    mov [bdb_heads], dh

    ; read FAT root directory

    mov ax, [bdb_sectors_per_fat]
    mov bl, [bdb_fat_count]
    xor bh, bh
    mul bx
    add ax, [bdb_reserved_sectors]

    push ax

    mov ax, [bdb_dir_entries_count]
    shl ax, 5
    xor dx, dx
    div word [bdb_bytes_per_sector]

    test dx, dx
    jz root_dir_after
    inc ax

root_dir_after:
    mov cl, al
    pop ax
    mov dl, [ebr_drive_number]
    mov bx, buffer
    call disk_read

    xor bx, bx
    mov di, buffer

search_kernel:
    mov si, file_kernel_bin
    mov cx, 11
    push di
    repe cmpsb
    pop di
    je found_kernel

    add di, 32
    inc bx
    cmp bx, [bdb_dir_entries_count]
    jl search_kernel

    jmp kernel_not_found


found_kernel:
    mov ax, [di + 26]
    mov [kernel_cluster], ax

    mov ax, [bdb_reserved_sectors]
    mov bx, buffer
    mov cl, [bdb_sectors_per_fat]
    mov dl, [ebr_drive_number]
    call disk_read
    
    mov bx, KERNEL_LOAD_SEGMENT
    mov es, bx
    mov bx, KERNEL_LOAD_OFFSET

load_kernel_loop:
    mov ax, [kernel_cluster]
    add ax, 31                          ; note: that this is hardcoded and only works for 1.44MB floppy disks I think 

    mov cl, 1
    mov dl, [ebr_drive_number]
    call disk_read

    add bx, [bdb_bytes_per_sector]

    mov ax, [kernel_cluster]
    mov cx, 3
    mul cx
    mov cx, 2
    div cx

    mov si, buffer
    add si, ax
    mov ax, [ds:si]
    or dx, dx
    jz even

odd:
    shr ax, 4
    jmp next_cluster_after

even: 
    and ax, 0x0FFF

next_cluster_after:
    cmp ax, 0x0FF8
    jae read_finish

    mov [kernel_cluster], ax
    jmp load_kernel_loop

read_finish:
    mov dl, [ebr_drive_number]
    mov ax, KERNEL_LOAD_SEGMENT
    mov ds, ax
    mov es, ax

    jmp KERNEL_LOAD_SEGMENT:KERNEL_LOAD_OFFSET

    jmp reboot_if_key
  

    cli
    hlt

; --------------------------------------------
; print string at DS:SI
print:
    push si
    push ax
    push bx

.loop:
    lodsb
    or al, al
    jz .exit
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    jmp .loop

.exit:
    pop bx
    pop ax
    pop si
    ret

; --------------------------------------------


; --------------------------------------------
floppy_error:
    mov si, floppy_error_msg
    call print
    jmp reboot_if_key

kernel_not_found:
    mov si, kernel_not_found_msg
    call print

reboot_if_key:
    mov ah, 0
    int 16h
    jmp 0FFFFh:0000h

; --------------------------------------------
; Converts LBA in AX to CHS in CH, CL, DH
lba_to_chs:
    push ax
    push dx

    xor dx, dx
    div word [bdb_sectors_per_track]   ; AX = track, DX = sector (0-based)
    inc dx                             ; sector starts at 1
    mov cx, dx                         ; CL = sector

    xor dx, dx
    div word [bdb_heads]              ; AX = cylinder, DX = head
    mov dh, dl                        ; DH = head
    mov ch, al                        ; CH = cylinder (lower 8 bits)
    shl ah, 6
    or cl, ah                         ; upper 2 bits of cylinder in CL bits 6-7

    pop ax
    mov dl, al
    pop ax
    ret

; --------------------------------------------
; Read from disk:
; AX = LBA
; CL = sector count
; BX = buffer
; DL = drive number
disk_read:
    push ax
    push bx
    push cx
    push dx
    push di

    push cx
    call lba_to_chs
    pop ax                      ; AL = number of sectors to read

    mov ah, 0x02                ; BIOS INT 13h function 2: read
    mov di, 3                   ; Retry 3 times

.retry:
    pusha
    stc
    int 13h
    jnc .done                   ; If no error, jump

    popa
    call disk_reset
    dec di
    test di, di
    jnz .retry

.fail:
    jmp floppy_error

.done:
    popa
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; --------------------------------------------
; Reset disk (DL = drive)
disk_reset:
    pusha
    mov ah, 0
    stc
    int 13h
    jc floppy_error
    popa
    ret

; --- Strings ---
loading_msg:            db 'Loading...', ENDL, 0
floppy_error_msg:       db 'Disk read failed', ENDL, 0
kernel_not_found_msg:   db 'stage2.bin not found', ENDL, 0
file_kernel_bin:        db 'STAGE2  BIN'
kernel_cluster:         dw 0

KERNEL_LOAD_SEGMENT     equ 0x2000
KERNEL_LOAD_OFFSET      equ 0

; --- Boot Signature ---
times 510 - ($ - $$) db 0
dw 0AA55h

buffer: 
