section .text
global _start

_start:
    mov rdx, 0
    mov ax, -100
    mov bl, 7
    idiv bl

    mov dx, -1
    mov ax, -300
    mov bx, 7
    idiv bx

    mov edx, 0
    mov eax, -100000
    mov ecx, 300
    idiv ecx

    mov rdx, -1
    mov rax, -9223372036854775807
    mov rcx, 123456789
    idiv rcx

    mov eax, -11
    mov ebx, 5
    idiv ebx

    mov eax, 42
    mov ebx, -3
    idiv ebx

    mov eax, -11
    mov ebx, 4
    idiv ebx

    mov dx, 0x7FFF
    mov ax, 0xFFFF
    mov bx, -1
    idiv bx

    hlt
