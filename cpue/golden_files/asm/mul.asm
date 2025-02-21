section .text
global _start

_start:
    mov rbx, 0x2
    mov al, 0x20
    mul bl

    mov rbx, 0xff
    mov al, 0xff
    mul bl

    mov rbx, 0xffff
    mov ax, 0xffff
    mul bx

    mov rbx, 0xffffffff
    mov eax, 0xffffffff
    mul ebx

    mov rbx, 0xffffffffffffffff
    mov rax, 0xffffffffffffffff
    mul rbx

    mov rax, 0xffffffff
    mul eax

    mov rax, 0xffffffff
    mul rax

    hlt
