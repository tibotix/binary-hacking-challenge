section .text
global _start

_start:
    mov ax, 100
    mov bl, 7
    div bl

    mov dx, 0
    mov ax, 300
    mov bx, 7
    div bx

    mov edx, 0
    mov eax, 100000
    mov ecx, 300
    div ecx

    mov rdx, 0
    mov rax, 9223372036854775807
    mov rcx, 123456789
    div rcx

    mov ax, 11
    mov bl, 5
    div bl

    mov ax, 42
    mov bl, 1
    div bl

    mov rax, 0xffffffffffffffff
    mov rdx, 0xffffffffffffcfff
    mov rbx, 0xffffffffffffffff
    div rbx

    mov dx, 0xFFFF
    mov ax, 0xFFFF
    mov bx, 256
    div bx

    mov ax, 100
    mov bl, 0
    div bl

    hlt
