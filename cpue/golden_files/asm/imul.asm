section .text
global _start

_start:
; 1-Operand form
    mov bl, -10
    mov al, -5
    imul bl

    mov bx, -100
    mov ax, -300
    imul bx

    mov ebx, -2
    mov eax, -2000000000
    imul ebx

; 2-Operand form
    mov rax, -9223372036854775807
    imul rax, -1

    mov eax, -1234
    imul eax, -5678

; 3-Operand form
    mov eax, -1234
    imul eax, eax, 10

; flags test
    mov eax, 0x7FFFFFFF
    imul eax, 2

    mov eax, 0x80000000
    imul eax, 2

    hlt
