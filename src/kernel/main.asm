org 0x0
bits 16

start:
  mov si, hello
  call print

  cli
  hlt

print:
    ; save registers we will modify
    push si
    push ax
    push bx

.loop:
    lodsb               ; loads next character in al
    or al, al           ; verify if next character is null?
    jz .done

    mov ah, 0x0E        ; call bios interrupt
    mov bh, 0           ; set page number to 0
    int 0x10

    jmp .loop

.done:
    pop bx
    pop ax
    pop si    
    ret

hello: db "Booting from Kernel", 0x0A, 0x0D, 0 
