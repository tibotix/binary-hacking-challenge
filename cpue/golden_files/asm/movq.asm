section .data
value1 dq 0x1122334455667788

section .bss
buffer resb 20


section .text
global _start

_start:
    ; === MOVQ von XMM nach GPR ===
    movq xmm0, qword [value1]  ; XMM0 = 0x1122334455667788
    movq rax, xmm0             ; RAX = XMM0 (untere 64 Bit)

    ; === MOVQ von GPR nach XMM ===
    mov rbx, 0xDEADBEEFCAFEBABE
    movq xmm1, rbx             ; XMM1 = RBX (Nullt obere 64 Bit)
    movq rax, xmm1             ; RAX = XMM1 (nur untere 64 Bit)

    ; === MOVQ zwischen XMM-Registern ===
    movq xmm2, xmm0            ; XMM2 = XMM0 (nur untere 64 Bit)
    movq rax, xmm2             ; RAX = XMM2 (nur untere 64 Bit)

    ; === MOVQ von XMM nach Speicher ===
    movq [buffer], xmm1        ; Speichert XMM1 in buffer
    movq xmm3, [buffer]        ; Lädt aus buffer nach XMM3
    movq rax, xmm3             ; RAX = XMM3


    hlt
