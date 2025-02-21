section .text
global _start

_start:
    mov rax, 0x1
    mov rcx, 0x0
    sub rcx, rax
    hlt
