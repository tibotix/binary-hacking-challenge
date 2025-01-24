section .text
global _start

_start:
    add rax, 0x1
    mov ebx, 0xdead
    mov bx, 0x2
    lea rdx, [8*ebx]
    lea rdx, [ebx + 8*ebx]
    sub rax, 0x1
    xor rbx, rbx
    mov rsi, QWORD ds:[hello]
    mov edi, _exit
    jmp rdi
    mov rax, 0xff
    hlt

_exit:
    mov rax, 0x0
    hlt


section .data
    hello dq 0xcafecafecafecafe
