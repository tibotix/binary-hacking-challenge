section .text
global _start

_start:
    mov rbx, -1
    mov ax, 0xffff
    movzx rbx, ax
    hlt
