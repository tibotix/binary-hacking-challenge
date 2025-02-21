section .data
    src_data db 'Hello, world!', 0
    src_len equ $ - src_data

section .bss
    dest_buffer resb 20


section .text
global _start

_start:
    mov rsi, src_data
    mov rdi, dest_buffer
    mov rcx, src_len
    cld
loop_movsb:
    movsb
    loop loop_movsb

    mov rsi, src_data
    mov rdi, dest_buffer
    mov rcx, src_len / 2
    cld
loop_movsw:
    movsw
    loop loop_movsw

    mov rsi, src_data
    mov rdi, dest_buffer
    mov rcx, src_len / 4
    cld
loop_movsd:
    movsd
    loop loop_movsd

    mov rsi, src_data
    mov rdi, dest_buffer
    mov rcx, src_len / 8
    cld
loop_movsq:
    movsq
    loop loop_movsq

    mov rsi, dest_buffer

    hlt
