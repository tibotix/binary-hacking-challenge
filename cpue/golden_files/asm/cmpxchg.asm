section .data
value dd 0x20

section .text
global _start

_start:
    mov eax, 0x10
    mov ebx, 0x10
    mov ecx, 0x20
    cmpxchg ebx, ecx

    mov eax, 0x15
    mov ebx, 0x10
    mov ecx, 0x20
    cmpxchg ebx, ecx

    mov eax, 0x10
    mov ecx, 0x30
    lock cmpxchg dword [value], ecx

    mov rax, 0x1234
    mov rbx, 0x1234
    mov rcx, 0x5678
    cmpxchg rbx, rcx

    mov al, 0xFF
    mov bl, 0xFF
    mov cl, 0x00
    cmpxchg bl, cl

    mov al, 0x10
    mov bl, 0x20
    mov cl, 0x30
    cmpxchg bl, cl

    hlt
