section .text
global _start

_start:
    add rax, 0x1
    mov rbx, 0xdeadbeefdeadbeef
    mov bx, 0x2
    lea rdx, [8*ebx]
    lea rdx, [ebx + 8*ebx]
    sub rax, 0x1
    xor rbx, rbx
    mov rsi, QWORD [hello]
    mov edi, _stack_test
    jmp rdi
    mov rax, 0xff
    hlt

_stack_test:
   push 0x0
   pop rax
   push rbx
   pop QWORD [hello]
   mov rsi, QWORD [hello]
   jmp _exit
   mov rax, 0xf0
   hlt

_exit:
    mov rax, 0x0
    hlt


section .data
    hello dq 0xcafecafecafecafe
