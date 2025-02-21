section .text
global _start

_start:
    mov eax, 0xffffffff
    movsxd rbx, eax
    hlt

