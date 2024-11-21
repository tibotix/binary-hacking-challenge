#pragma once

#include "mmu.h"
#include "address.h"
#include "disassembler.h"
#include "pic.h"


namespace CPUE {

// TODO: when implementing protected instructions, see chapter 2.8: System Instruction Summary



class CPU {
public:
    friend MMU;
    friend Disassembler;
    CPU() : m_mmu{this, 2}, m_disassembler(this) {}
    CPU(CPU const&) = delete;

public:
    enum ExecutionMode {
        REAL_MODE,
        LEGACY_MODE,
        PROTECTED_MODE,
        COMPATIBILITY_MODE,
        LONG_MODE,
    };

public:
    bool is_paging_enabled() {
        TODO_NOFAIL("is_paging_enabled: use actual registers");
        return true;
    }
    inline void assert_paging_enabled() { CPUE_ASSERT(is_paging_enabled(), "paging not enabled"); }
    ExecutionMode execution_mode() {
        if (m_efer.LMA == 1)
            return ExecutionMode::LONG_MODE;
        TODO_NOFAIL("execution_mode");
        return ExecutionMode::COMPATIBILITY_MODE;
    }
    inline void assert_in_long_mode() { CPUE_ASSERT(execution_mode() == ExecutionMode::LONG_MODE, "not in long mode"); }

    void interpreter_loop();

    MMU& mmu() { return m_mmu; }
    PIC& pic() { return m_pic; }

private:
    InterruptRaisedOr<void> do_canonicality_check(VirtualAddress const& vaddr);

private:
    MMU m_mmu;
    PIC m_pic;
    Disassembler m_disassembler;

    // TODO: initialize all registers with sane default values

    // General Purpose / Pointer Registers
    u64 m_rax;
    u64 m_rbx;
    u64 m_rcx;
    u64 m_rdx;
    u64 m_rsi;
    u64 m_rsp;
    u64 m_rbp;
    u64 m_rip;
    u64 m_r8;
    u64 m_r9;
    u64 m_r10;
    u64 m_r11;
    u64 m_r12;
    u64 m_r13;
    u64 m_r14;
    u64 m_r15;


    /**
     * Segment Registers:
     * In 64-bit mode: CS, DS, ES, SS are treated as if each segment base is 0, regardless of the value of the associated
     * segment descriptor base. This creates a flat address space for code, data, and stack. FS and GS are exceptions.
     * Both segment registers may be used as additional base registers in linear address calculations
     */
    struct SegmentRegister {
        // visible part:
        SegmentSelector segment_selector{0};
        /**
         * hidden part (shadow register):
         * When a segment selector is loaded into the visible part of a segment
         * register, the processor also loads the hidden part of the segment register with the base address, segment limit, and
         * access control information from the segment descriptor pointed to by the segment selector. The information cached
         * in the segment register (visible and hidden) allows the processor to translate addresses without taking extra bus
         * cycles to read the base address and limit from the segment descriptor
         * 
         */
        u32 base;
        u32 limit;
    };
    SegmentRegister m_cs;
    SegmentRegister m_ds;
    SegmentRegister m_ss;
    SegmentRegister m_es;
    SegmentRegister m_fs;
    SegmentRegister m_gs;


    /**
     * RFLAGS Registers
     */
    struct RFLAGS {
        u64 CF : 1; // Carry Flag
        u8 : 1; // Reserved
        u8 PF : 1; // Parity Flag
        u8 : 1; // Reserved
        u8 AF : 1; // Auxiliary Carry Flag
        u8 : 1; // Reserved
        u8 ZF : 1; // Zero Flag
        u8 SF : 1; // Sign Flag
        u8 TF : 1; // Trap Flag
        u8 IF : 1; // Interrupt Enable Flag
        u8 DF : 1; // Direction Flag
        u8 OF : 1; // Overflow Flag
        u8 IOPL : 2; // I/O Privilege Leve
        u8 NT : 1; // Nested Task
        u8 : 1; // Reserved
        u8 RF : 1; // Resume Flag
        u8 VM : 1; // Virtual-8086 Mode
        u8 AC : 1; // Alignment Check / Access Control
        u8 VIF : 1; // Virtual Interrupt Flag
        u8 VIP : 1; // Virtual Interrupt Pending
        u8 ID : 1; // ID Flag
    };
    RFLAGS m_rflags;


    /**
     * Control Registers
     */
    struct CR0 {
        u64 PE : 1; // Protected Mode Enable
        u8 MP : 1; // Monitor Co-Processor
        u8 EM : 1; // Emulation
        u8 TS : 1; // Task Switched
        u8 ET : 1; // Extension Type
        u8 NE : 1; // Numeric Error
        u16 : 10; // Reserved
        u8 WP : 1; // Write Protect
        u8 : 1; // Reserved
        u8 AM : 1; // Alignment Mask
        u16 : 10; // Reserved
        u8 NW : 1; // Not-Write Through
        u8 CD : 1; // Cache Disable
        u8 PG : 1; // Paging
    };
    CR0 m_cr0;
    // This control register contains the linear (virtual) address which triggered a page fault, available in the page fault's interrupt handler.
    VirtualAddress m_cr2;
    struct CR3 {
        union _PCID {
            struct {
                u16 : 2;
                u8 PWT : 1; // Page-Level Write Through
                u8 PCD : 1; // Page-Level Write-Through
                u8 : 7; // Padding to 11 bits
            } NO_PCIDE;
            u64 PCIDE : 11;
        } pcid;
        // _PCID pcid : 11;
        u64 pml4_base_paddr : 52; // Physical Base Address of the PML4
        u8 LAM_U57 : 1; // When set, enables LAM57 (masking of linear-address bits 62:57) for user pointers and overrides CR3.LAM_U48.
        u8 LAM_U48 : 1; // When set and CR3.LAM_U57 is clear, enables LAM48 (masking of linear-address bits 62:48) for user pointers.
    };
    CR3 m_cr3;
    struct CR4 {
        u64 VME : 1; // Virtual-8086 Mode Extensions
        u8 PVI : 1; // Protected Mode Virtual Interrupts
        u8 TSD : 1; // Time Stamp enabled only in ring 0
        u8 DE : 1; // Debugging Extensions
        u8 PSE : 1; // Page Size Extension
        u8 PAE : 1; // Physical Address Extension
        u8 MCE : 1; // Machine Check Exception
        u8 PGE : 1; // Page Global Enable
        u8 PCE : 1; // Performance Monitoring Counter Enable
        u8 OSFXSR : 1; // OS support for fxsave and fxrstor instructions
        u8 OSXMMEXCPT : 1; // OS Support for unmasked simd floating point exceptions
        u8 UMIP : 1; // User-Mode Instruction Prevention (SGDT, SIDT, SLDT, SMSW, and STR are disabled in user mode)
        u8 LA57 : 1 = 0; // 57-bit linear addresses. When set in IA-32e mode, the processor uses 5-level paging to translate 57-bit linear addresses. When clear in IA-32e mode, the processor uses 4-level paging to translate 48-bit linear addresses.
        u8 VMXE : 1; // Virtual Machine Extensions Enable
        u8 SMXE : 1; // Safer Mode Extensions Enable
        u8 : 1; // Reserved
        u8 FSGSBASE : 1; // Enables the instructions RDFSBASE, RDGSBASE, WRFSBASE, and WRGSBASE
        u8 PCIDE : 1; // PCID Enable
        u8 OSXSAVE : 1; // XSAVE And Processor Extended States Enable
        u8 KL : 1 = 0; // Key-Locker-Enable Bit.
        u8 SMEP : 1; // Supervisor Mode Executions Protection Enable
        u8 SMAP : 1; // Supervisor Mode Access Protection Enable
        u8 PKE : 1; // Enable protection keys for user-mode pages
        u8 CET : 1; // Enable Control-flow Enforcement Technology
        u8 PKS : 1; // Enable protection keys for supervisor-mode pages
        u8 UINTR : 1; // Enables user interrupts when set, including user-interrupt delivery, user-interrupt notification identification, and the user-interrupt instructions.
        u8 : 2; // Reserved
        u8 LAM_SUP : 1; // When set, enables LAM (linear-address masking) for supervisor pointers.
    };
    CR4 m_cr4;
    struct CR8 {
        u64 TPR : 4; // This sets the threshold value corresponding to the highestpriority interrupt to be blocked. A value of 0 means all interrupts are enabled. This field is available in 64-bit mode. A value of 15 means all interrupts will be disabled.
    };
    CR8 m_cr8;
    /**
     * CR1, CR5-7, CR9-15:
     * Reserved, the cpu will throw a #ud exception when trying to access them.
     */


    /**
     * MSRs (Model-Specific-Registers)
     */
    struct EFER {
        u64 SCE : 1; // System Call Extensions
        u8 : 7; // Reserved
        u8 LME : 2; // Long Mode Enable
        u8 LMA : 1; // Long Mode Active
        u8 NXE : 1; // No-Execute Enable
        u8 SVME : 1; // Secure Virtual Machine Enable
        u8 LMSLE : 1; // Long Mode Segment Limit Enable
        u8 FFXSR : 1; // Fast FXSAVE/FXRSTOR
        u8 TCE : 1; // Translation Cache Extension u8 16 - 63 0 Reserved
    };
    EFER m_efer;
    // MSRs with the addresses 0xC0000100 (for FS) and 0xC0000101 (for GS) contain the base addresses of the FS and GS segment registers.
    // These are commonly used for thread-pointers in user code and CPU-local pointers in kernel code.
    // Safe to contain anything, since use of a segment does not confer additional privileges to user code.
    // In newer CPUs, these can also be written with WRFSBASE and WRGSBASE instructions at any privilege level.
    VirtualAddress m_fsbase;
    VirtualAddress m_gsbase;
    // MSR with the address 0xC0000102.
    // Is basically a buffer that gets exchanged with GS.base after a swapgs instruction.
    // Usually used to separate kernel and user use of the GS register.
    VirtualAddress m_kernel_gsbase;


    /**
     * Debug Registers:
     * Not Implemented!
     */


    /**
     * Test Registers
     * Not Implemented!
     */


    /**
     * Protected Mode Registers
     * As with segments, the limit value is added to the base address to get the address of the last valid byte. 
     */
    struct GDTR {
        VirtualAddress base = 0_va; // The base address specifies the linear address of byte 0 of the GDT
        u16 limit = 0x0FFFF; // The table limit specifies the number of bytes in the table.
    };
    GDTR m_gdtr;
    struct LDTR {
        VirtualAddress base = 0_va; // The base address specifies the linear address of byte 0 of the LDT segment
        u16 limit = 0x0FFFF; // The segment limit specifies the number of bytes in the segment
        u16 segment_selector;
    };
    LDTR m_ldtr;
    struct IDTR {
        VirtualAddress base = 0_va; // The base address specifies the linear address of byte 0 of the IDT
        u16 limit = 0x0FFFF; // The table limit specifies the numberof bytes in the table
    };
    IDTR m_idtr;
    struct TR {
        VirtualAddress base = 0_va; // The base address specifies the linear address of byte 0 of the TSS
        u16 limit = 0x0FFFF; // The segment limit specifies the number of bytes in the TSS
        u16 segment_selector;
    };
    TR m_tr;
};


}