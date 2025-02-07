section .text
global _start

_start:
    ; Test 1: SHR mit 8-Bit-Wert (1 Byte), Shift-Wert 3
    mov al, 0b10000000    ; al = 128 (binär: 10000000)
    shr al, 3             ; al = al >> 3 (Shift um 3 Bits)

    ; Test 2: SHR mit 32-Bit-Wert (4 Byte), Shift-Wert 16
    mov eax, 0b10000000000000000000000000000000 ; eax = 2147483648 (binär: 10000000000000000000000000000000)
    shr eax, 16           ; eax = eax >> 16 (Shift um 16 Bits)

    ; Test 3: SHR mit 64-Bit-Wert (8 Byte), Shift-Wert 32
    mov rax, 0b1000000000000000000000000000000000000000000000000000000000000000 ; rax = 9223372036854775808 (binär: 1000000000000000000000000000000000000000000000000000000000000000)
    shr rax, 32           ; rax = rax >> 32 (Shift um 32 Bits)

    ; Test 1: SHR mit 8-Bit-Wert (1 Byte), großer Shift-Wert
    mov al, 0b10000000    ; al = 128 (binär: 10000000)
    shr al, 8             ; al = al >> 8 (shift mit einem Wert > 8 Bits)

    ; Test 2: SHR mit 32-Bit-Wert (4 Byte), großer Shift-Wert
    mov eax, 0b10000000000000000000000000000000 ; eax = 2147483648 (binär: 10000000000000000000000000000000)
    shr eax, 32            ; eax = eax >> 32 (shift mit einem Wert > 32 Bits)

    ; Test 3: SHR mit 64-Bit-Wert (8 Byte), großer Shift-Wert
    mov rax, 0b1000000000000000000000000000000000000000000000000000000000000000 ; rax = 9223372036854775808 (binär: 1000000000000000000000000000000000000000000000000000000000000000)
    shr rax, 67            ; rax = rax >> 64 (shift mit einem Wert > 64 Bits)

    hlt
