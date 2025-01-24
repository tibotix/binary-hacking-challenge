
Example 64-bit assembly
```asm
section .text
global _start

_start:
    mov rdi, 0x1
    mov rsi, hello
    mov rdx, helloLen
    mov rax, 0x1
    syscall

    xor rdi, rdi
    mov rax, 0x3c
    syscall

section .data
    hello db "Hello World", 0xa
    helloLen equ $-hello
```

compile it with nasm and ld
```sh
nasm -f elf64 -o simple_add.o simple_add.asm
ld -o simple_add -no-pie -static simple_add.o
# with --section-start=.text=... to set base address
```

run with
```sh
./build/cpue -v --kernel=none --no-serial ./golden_files/demo
```
