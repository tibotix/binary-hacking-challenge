section .text
global _start

_start:
    mov eax, 0x1
    cqo

    mov rax, 0xff00000000000000
    cqo

    mov rax, 0x0fffffffffffffff
    cqo
    hlt
