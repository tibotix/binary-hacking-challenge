section .text
global _start

_start:
    mov rcx, 0x20
    xor rax,rax
loop_start:
    add rax, 1
    loop loop_start
    hlt
