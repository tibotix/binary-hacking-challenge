%macro save_user_regs 0
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rsi
    push rdi
    push rdx
    push rcx
    push rbp
    push rbx
    push rax
%endmacro
%macro restore_user_regs 0
    pop rax
    pop rbx
    pop rbp
    pop rcx
    pop rdx
    pop rdi
    pop rsi
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
%endmacro
%define saved_user_regs_size (15*8)


extern isr_table

section .text
global ihandler_tramp



ihandler:
    save_user_regs

    mov rbx, [rsp + saved_user_regs_size] ; load irq
    lea rax, [isr_table + (rbx*0x8)] ; load struct isr address
    mov rsi, rsp ; load pointer to saved_user_regs
    mov rdi, [rsp + saved_user_regs_size + 0x8] ; store error_code

    ; we now call the specified isr-handler.
    ; it can freely manipulate the saved-user-regs passed in rsi
    call qword [rax] ; call isr->handler

    restore_user_regs

    ; now skip irq + error_code and setup rsp for iret instruction
    add rsp, 0x10
    sti
    iretq



%macro jmp_to_ihandler_with_error_code 1
    push %1
    jmp ihandler
    align 0x10
%endmacro
%macro jmp_to_ihandler_without_error_code 1
    push 0x0 ; push fake error_code to be consistent
    push %1
    jmp ihandler
    align 0x10
%endmacro
%macro unused_irq 0
    hlt
    align 0x10
%endmacro

align 0x10 ; align ihandler_tramp to 0x10 boundary
ihandler_tramp:
    jmp_to_ihandler_without_error_code 0 ; DE
    jmp_to_ihandler_without_error_code 1; DB
    jmp_to_ihandler_without_error_code 2 ; NMI
    jmp_to_ihandler_without_error_code 3 ; BP
    jmp_to_ihandler_without_error_code 4 ; OF
    jmp_to_ihandler_without_error_code 5 ; BR
    jmp_to_ihandler_without_error_code 6 ; UD
    jmp_to_ihandler_without_error_code 7 ; NM
    jmp_to_ihandler_with_error_code 8 ; DF
    unused_irq ; Coprocessor Segment Overrun
    jmp_to_ihandler_with_error_code 10 ; TS
    jmp_to_ihandler_with_error_code 11 ; NP
    jmp_to_ihandler_with_error_code 12 ; SS
    jmp_to_ihandler_with_error_code 13 ; GP
    jmp_to_ihandler_with_error_code 14 ; PF
    unused_irq ; Intel reserved
    jmp_to_ihandler_without_error_code 16 ; MF
    jmp_to_ihandler_with_error_code 17 ; AC
    jmp_to_ihandler_without_error_code 18 ; MC
    jmp_to_ihandler_without_error_code 19 ; XM
    jmp_to_ihandler_without_error_code 20 ; VE
    jmp_to_ihandler_with_error_code 21 ; CP

    ; Intel reserved interrupts (irqs 22-31)
    %assign i 22
    %rep    31-22
        unused_irq
    %assign i i+1
    %endrep

    ; User Defined Interrupts
    %rep    255-32
        jmp_to_ihandler_without_error_code i
    %assign i i+1
    %endrep


