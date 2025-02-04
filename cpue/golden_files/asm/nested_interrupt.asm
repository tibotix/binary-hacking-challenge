
section .text
global _start

_start:
    mov rax, qword [0xfffffff0000000]
    hlt

