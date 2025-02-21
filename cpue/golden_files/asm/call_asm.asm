section .text
global _start

_start:
    call func1
    call func1
    add rcx, 0x1
    hlt

func1:
    push rbp
    mov rbp, rsp
    add rax, 0x1
    call func2
    leave
    ret

func2:
    push rbp
    mov rbp, rsp
    add rbx, 0x1
    leave
    ret
