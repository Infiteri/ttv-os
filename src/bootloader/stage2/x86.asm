bits 16

section _TEXT class=CODE

global __U4D
__U4D:
    shl edx, 16         
    mov dx, ax          
    mov eax, edx        
    xor edx, edx

    shl ecx, 16         
    mov cx, bx          

    div ecx             
    mov ebx, edx
    mov ecx, edx
    shr ecx, 16

    mov edx, eax
    shr edx, 16

    ret

global __U4M
__U4M:
    shl edx, 16         ; dx to upper half of edx
    mov dx, ax          ; m1 in edx
    mov eax, edx        ; m1 in eax

    shl ecx, 16         ; cx to upper half of ecx
    mov cx, bx          ; m2 in ecx

    mul ecx             ; result in edx:eax (we only need eax)
    mov edx, eax        ; move upper half to dx
    shr edx, 16

    ret

global _x86_div64_32
_x86_div64_32:
    push bp
    mov bp, sp

    push bx

    mov eax, [bp + 8]
    mov ecx, [bp + 12]
    xor edx, edx
    div ecx

    mov bx, [bp + 16]
    mov [bx], eax

    mov eax, [bp + 4]
  
    div ecx

    mov [bx], eax
    mov bx, [bp + 18]
    mov [bx], edx

    pop bx
    
    ; Move stack frame 
    mov sp, bp
    pop bp
    ret


global _x86_Video_WriteCharTeletype
_x86_Video_WriteCharTeletype:
    push bp
    mov bp, sp
    
    push bx

    mov ah, 0Eh
    mov al, [bp + 4]
    mov bh, [bp + 6]
    
    int 10h

    pop bx

    ; Move stack frame 
    mov sp, bp
    pop bp
    ret

global _x86_Disk_Reset
_x86_Disk_Reset:
    push bp
    mov bp, sp

    mov ah, 0
    mov dl, [bp + 4]
    stc
    int 13h

    mov ax, 1
    sbb ax, 0
    

    ; Move stack frame 
    mov sp, bp
    pop bp
    ret

global _x86_Disk_Read
_x86_Disk_Read:

    ; make new call frame
    push bp             ; save old call frame
    mov bp, sp          ; initialize new call frame

    ; save modified regs
    push bx
    push es

    ; setup args
    mov dl, [bp + 4]    ; dl - drive

    mov ch, [bp + 6]    ; ch - cylinder (lower 8 bits)
    mov cl, [bp + 7]    ; cl - cylinder to bits 6-7
    shl cl, 6
    
    mov al, [bp + 8]    ; cl - sector to bits 0-5
    and al, 3Fh
    or cl, al

    mov dh, [bp + 10]   ; dh - head

    mov al, [bp + 12]   ; al - count

    mov bx, [bp + 16]   ; es:bx - far pointer to data out
    mov es, bx
    mov bx, [bp + 14]

    ; call int13h
    mov ah, 02h
    stc
    int 13h

    ; set return value
    mov ax, 1
    sbb ax, 0           ; 1 on success, 0 on fail   

    ; restore regs
    pop es
    pop bx

    ; restore old call frame
    mov sp, bp
    pop bp
    ret

global _x86_Disk_GetDriveParams
_x86_Disk_GetDriveParams:
    push bp
    mov bp, sp

    push es
    push bx
    push si
    push di

    mov dl, [bp + 4]
    mov ah, 08h
    mov di, 0
    mov es, di
    stc
    int 13h

    mov ax, 1
    sbb ax, 0   

    mov si, [bp + 6]
    mov [si], bl

    mov bl, ch
    mov bh, cl
    shr bh, 6

    mov si, [bp + 8]
    mov [si], bx

    xor ch, ch
    and cl, 3Fh
    mov si, [bp + 10]
    mov [si], cx

    mov cl, dh
    mov si, [bp + 12]
    mov [si], cx

    pop di
    pop si
    pop bx
    pop es
    
    ; Move stack frame 
    mov sp, bp
    pop bp
    ret


