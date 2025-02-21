section .text
global _start

_start:
    ; Test 1: SHL mit 8-Bit-Wert (1 Byte)
    mov al, 0b00000001    ; al = 1 (binär: 00000001)
    shl al, 2             ; al = al << 2 -> al = 4 (binär: 00000100)

    ; Test 2: SHL mit 32-Bit-Wert (4 Byte)
    mov eax, 0b00000000000000000000000000000001 ; eax = 1 (binär: 00000000000000000000000000000001)
    shl eax, 5             ; eax = eax << 5 -> eax = 32 (binär: 00000000000000000000000000100000)

    ; Test 3: SHL mit 64-Bit-Wert (8 Byte)
    mov rax, 0b0000000000000000000000000000000000000000000000000000000000000001 ; rax = 1 (binär: 00000001)
    shl rax, 10            ; rax = rax << 10 -> rax = 1024 (binär: 00000000000000000000010000000000)

    ; Test 1: SHL mit 8-Bit-Wert (1 Byte), großer Shift-Wert
    mov al, 0b00000001    ; al = 1 (binär: 00000001)
    shl al, 9             ; al = al << 8 (shift mit einem Wert > 8 Bits)

    ; Test 2: SHL mit 32-Bit-Wert (4 Byte), großer Shift-Wert
    mov eax, 0b00000000000000000000000000000001 ; eax = 1 (binär: 00000000000000000000000000000001)
    shl eax, 32            ; eax = eax << 32 (shift mit einem Wert > 32 Bits)

    ; Test 3: SHL mit 64-Bit-Wert (8 Byte), großer Shift-Wert
    mov rax, 0b0000000000000000000000000000000000000000000000000000000000000001 ; rax = 1 (binär: 00000001)
    shl rax, 65            ; rax = rax << 64 (shift mit einem Wert > 64 Bits)

    hlt

