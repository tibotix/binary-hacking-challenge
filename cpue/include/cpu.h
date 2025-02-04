#pragma once

#include <stack>
#include <map>

#include "mmu.h"
#include "controllers/icu.h"
#include "controllers/pic.h"
#include "controllers/uart.h"
#include "address.h"
#include "disassembler.h"
#include "segmentation.h"
#include "forward.h"
#include "register_proxy.h"
#include "spdlog/fmt/bundled/chrono.h"


namespace CPUE {

// TODO: when implementing protected instructions, see chapter 2.8: System Instruction Summary
// TODO: when implementing some instructions, look at type checking (chapter 6.4 or page 3250)


class CPU {
public:
    friend MMU;
    friend Disassembler;
    friend TLB;
    friend ICU;
    friend UEFI;
    friend RegisterProxy;
    friend GeneralPurposeRegisterProxy;
    friend ControlRegisterProxy;
    friend ApplicationSegmentRegisterProxy;
    friend class NoKernel;
    friend class EmulatedKernel;
    friend class CustomKernel;
    explicit CPU(size_t available_pages = 2) : m_icu(this), m_mmu(this, available_pages), m_pic(PIC{this}), m_disassembler(Disassembler{this}), m_uart1(UARTController{this}) {
        reset();
    }
    CPU(CPU const&) = delete;

public:
    enum ExecutionMode {
        REAL_MODE,
        LEGACY_MODE,
        PROTECTED_MODE,
        COMPATIBILITY_MODE,
        LONG_MODE,
    };

    enum State {
        STATE_FETCH_INSTRUCTION,
        STATE_HANDLE_INSTRUCTION,
        STATE_HANDLE_INTERRUPT,
        STATE_HALTED,
    };

    enum PrivilegeMode {
        USER_MODE,
        SUPERVISOR_MODE,
    };

    enum IPIncrementBehavior {
        INCREMENT_IP,
        DONT_INCREMENT_IP,
    };

    enum PagingMode { PAGING_MODE_NONE, PAGING_MODE_32BIT, PAGING_MODE_PAE, PAGING_MODE_4LEVEL, PAGING_MODE_5LEVEL };

    union RFLAGS {
        struct Concrete {
            u64 CF : 1; // Carry Flag
            u64 : 1; // Reserved
            u64 PF : 1; // Parity Flag
            u64 : 1; // Reserved
            u64 AF : 1; // Auxiliary Carry Flag
            u64 : 1; // Reserved
            u64 ZF : 1; // Zero Flag
            u64 SF : 1; // Sign Flag
            u64 TF : 1; // Trap Flag
            u64 IF : 1; // Interrupt Enable Flag
            u64 DF : 1; // Direction Flag
            u64 OF : 1; // Overflow Flag
            u64 IOPL : 2; // I/O Privilege Leve
            u64 NT : 1; // Nested Task
            u64 : 1; // Reserved
            u64 RF : 1; // Resume Flag
            u64 VM : 1; // Virtual-8086 Mode
            // Controls alignment-checking in user-mode
            // Allows to disable SMAP for explicit accesses, can only be modified in supervisor mode
            u64 AC : 1; // Alignment Check / Access Control
            u64 VIF : 1; // Virtual Interrupt Flag
            u64 VIP : 1; // Virtual Interrupt Pending
            u64 ID : 1; // ID Flag
        } c;
        u64 value;
    };
    static_assert(sizeof(RFLAGS) == 8);

    // Control Registers
    union CR0 {
        struct Concrete {
            u64 PE : 1; // Protected Mode Enable
            u64 MP : 1; // Monitor Co-Processor
            u64 EM : 1; // Emulation
            u64 TS : 1; // Task Switched
            u64 ET : 1; // Extension Type
            u64 NE : 1; // Numeric Error
            u64 : 10; // Reserved
            u64 WP : 1; // Write Protect
            u64 : 1; // Reserved
            u64 AM : 1; // Alignment Mask
            u64 : 10; // Reserved
            u64 NW : 1; // Not-Write Through
            u64 CD : 1; // Cache Disable
            u64 PG : 1; // Paging
        } c;
        u64 value;
    };
    static_assert(sizeof(CR0) == 8);

    union CR3 {
        struct Concrete {
            u64 _pcid1 : 3;
            u64 PWT : 1;
            u64 PCD : 1;
            u64 _pcid2 : 7;
            u64 pml4_base_paddr : MAXPHYADDR - 12; // Physical Base Address of the PML4
            u64 __reserved1 : 60 - MAXPHYADDR = 0; // Reserved (must be 0)
            u64 LAM_U57 : 1; // When set, enables LAM57 (masking of linear-address bits 62:57) for user pointers and overrides CR3.LAM_U48.
            u64 LAM_U48 : 1; // When set and CR3.LAM_U57 is clear, enables LAM48 (masking of linear-address bits 62:48) for user pointers.
            u64 __reserved2 : 1 = 0; // Reserved (must be 0)
        } c;
        u64 value;

        u64 reserved_bits_ored() const { return c.__reserved1 || c.__reserved2; }

        // // We have to do it this way cause of bit-field limitations/alignment issues
        PCID pcid() const { return ((u16)c._pcid2 << 5) || c.PCD << 4 || c.PWT << 3 || c._pcid1; }
        void set_pcid(PCID pcid) {
            c._pcid1 = bits(pcid, 2, 0);
            c.PWT = bits(pcid, 3, 3);
            c.PCD = bits(pcid, 4, 4);
            c._pcid2 = bits(pcid, 11, 5);
        }
    };
    static_assert(sizeof(CR3) == 8);

    union CR4 {
        struct Concrete {
            u64 VME : 1; // Virtual-8086 Mode Extensions
            u64 PVI : 1; // Protected Mode Virtual Interrupts
            u64 TSD : 1; // Time Stamp enabled only in ring 0
            u64 DE : 1; // Debugging Extensions
            u64 PSE : 1; // Page Size Extension
            u64 PAE : 1; // Physical Address Extension
            u64 MCE : 1; // Machine Check Exception
            u64 PGE : 1; // Page Global Enable
            u64 PCE : 1; // Performance Monitoring Counter Enable
            u64 OSFXSR : 1; // OS support for fxsave and fxrstor instructions
            u64 OSXMMEXCPT : 1; // OS Support for unmasked simd floating point exceptions
            u64 UMIP : 1; // User-Mode Instruction Prevention (SGDT, SIDT, SLDT, SMSW, and STR are disabled in user mode)
            u64 LA57 : 1 = 0; // 57-bit linear addresses. When set in IA-32e mode, the processor uses 5-level paging to translate 57-bit linear addresses. When clear in IA-32e mode, the processor uses 4-level paging to translate 48-bit linear addresses.
            u64 VMXE : 1; // Virtual Machine Extensions Enable
            u64 SMXE : 1; // Safer Mode Extensions Enable
            u64 __reserved1 : 1 = 0; // Reserved (must be 0)
            u64 FSGSBASE : 1; // Enables the instructions RDFSBASE, RDGSBASE, WRFSBASE, and WRGSBASE
            u64 PCIDE : 1; // PCID Enable
            u64 OSXSAVE : 1; // XSAVE And Processor Extended States Enable
            u64 KL : 1 = 0; // Key-Locker-Enable Bit.
            u64 SMEP : 1; // Supervisor Mode Executions Protection Enable
            u64 SMAP : 1; // Supervisor Mode Access Protection Enable
            u64 PKE : 1; // Enable protection keys for user-mode pages
            u64 CET : 1; // Enable Control-flow Enforcement Technology
            u64 PKS : 1; // Enable protection keys for supervisor-mode pages
            u64 UINTR : 1; // Enables user interrupts when set, including user-interrupt delivery, user-interrupt notification identification, and the user-interrupt instructions.
            u64 __reserved2 : 2 = 0; // Reserved (must be 0)
            u64 LAM_SUP : 1; // When set, enables LAM (linear-address masking) for supervisor pointers.
        } c;
        u64 value;

        u64 reserved_bits_ored() const { return c.__reserved1 || c.__reserved2; }
    };
    static_assert(sizeof(CR4) == 8);

    union CR8 {
        struct Concrete {
            u64 TPR : 4; // This sets the threshold value corresponding to the highestpriority interrupt to be blocked. A value of 0 means all interrupts are enabled. This field is available in 64-bit mode. A value of 15 means all interrupts will be disabled.
        } c;
        u64 value;
    };
    static_assert(sizeof(CR8) == 8);

    // Model Specific Registers
    union EFER {
        struct Concrete {
            u64 SCE : 1; // System Call Extensions
            u64 : 7; // Reserved
            u64 LME : 2; // Long Mode Enable
            u64 : 1; // Long Mode Active (See efer_LMA)
            u64 NXE : 1; // No-Execute Enable
            u64 SVME : 1; // Secure Virtual Machine Enable
            u64 LMSLE : 1; // Long Mode Segment Limit Enable
            u64 FFXSR : 1; // Fast FXSAVE/FXRSTOR
            u64 TCE : 1; // Translation Cache Extension u8 16 - 63 0 Reserved
        } c;
        u64 value;
    };


public:
    [[nodiscard]] PagingMode paging_mode() const;
    [[nodiscard]] bool is_paging_enabled() const {
        // We only support 4-Level paging or No paging.
        switch (paging_mode()) {
            case PAGING_MODE_NONE: return false;
            case PAGING_MODE_4LEVEL: return true;
            case PAGING_MODE_32BIT:
            case PAGING_MODE_PAE:
            case PAGING_MODE_5LEVEL: fail("Only 4-Level Paging or No Paging is supported.");
        }
    }
    void assert_paging_enabled() const { CPUE_ASSERT(is_paging_enabled(), "paging not enabled"); }
    [[nodiscard]] ExecutionMode execution_mode() const;
    void assert_in_long_mode() const { CPUE_ASSERT(execution_mode() == ExecutionMode::LONG_MODE, "not in long mode"); }

    [[nodiscard]] State state() const { return m_state; }

    void interpreter_loop();
    [[noreturn]] void shutdown() {
        printf("Shutting down...\n");
        exit(0);
    }
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_insn(cs_insn const&);

    std::string dump_full_state() const;

    void reset();

    [[nodiscard]] u8 cpl() const { return m_cs.visible.segment_selector.c.rpl; }
    [[nodiscard]] PrivilegeMode cpm() const { return cpl() == 3 ? USER_MODE : SUPERVISOR_MODE; }
    void set_cpl(u8 cpl) { m_cs.visible.segment_selector.c.rpl = cpl; }

    /**
     * Process-Context Identifiers (PCIDs) (See page 3230)
     * A PCID is a 12-bit identifier. Non-zero PCIDs are enabled by setting the PCIDE flag (bit 17) of CR4. If CR4.PCIDE =
     * 0, the current PCID is always 000H; otherwise, the current PCID is the value of bits 11:0 of CR3.1 Not all processors
     * allow CR4.PCIDE to be set to 1;
     */
    [[nodiscard]] PCID pcid() const {
        if (cr4().c.PCIDE == 0)
            return 0;
        return cr3().pcid();
    }

    u64 rip() const { return m_rip_val; }
    void set_rip(u64 index) { m_rip_val = index; }


    CR0 cr0() const { return {.value = m_cr0_val}; }
    VirtualAddress cr2() const { return {m_cr2_val}; }
    CR3 cr3() const { return {.value = m_cr3_val}; }
    CR4 cr4() const { return {.value = m_cr4_val}; }
    CR8 cr8() const { return {.value = m_cr8_val}; }


    LogicalAddress stack_pointer() const { return {m_ss, m_rsp_val}; }
    [[nodiscard]] InterruptRaisedOr<void> stack_push(u64 value, TranslationIntention intention = INTENTION_UNKNOWN);
    [[nodiscard]] InterruptRaisedOr<u64> stack_pop(TranslationIntention intention = INTENTION_UNKNOWN);

    LogicalAddress code_pointer() const { return {m_cs, m_rip_val}; }


    [[nodiscard]] InterruptRaisedOr<void> load_segment_register(SegmentRegisterAlias alias, SegmentSelector selector);
    [[nodiscard]] InterruptRaisedOr<void> load_segment_register(SegmentRegisterAlias alias, SegmentSelector selector, GDTLDTDescriptor const& descriptor);


    RegisterProxy* reg(x86_reg reg) {
        CPUE_ASSERT(reg < sizeof(m_register_map), "reg out of range.");
        return m_register_map[reg];
    }
    RegisterProxy* gpreg(x86_reg reg) {
        auto r = this->reg(reg);
        CPUE_ASSERT(r == nullptr || r->type() == GENERAL_PURPOSE_REGISTER, "gpreg called with non-gp register.");
        return r;
    }
    RegisterProxy* sreg(x86_reg reg) {
        auto r = this->reg(reg);
        CPUE_ASSERT(r == nullptr || r->type() == APPLICATION_SEGMENT_REGISTER || r->type() == SYSTEM_SEGMENT_REGISTER, "sreg called with non-segment register.");
        return r;
    }
    RegisterProxy* creg(x86_reg reg) {
        auto r = this->reg(reg);
        CPUE_ASSERT(r == nullptr || r->type() == CONTROL_REGISTER, "creg called with non-control register.");
        return r;
    }


    RFLAGS& rflags() { return m_rflags; }
    EFER& efer() { return m_efer; }
    /**
     * The LMA flag in the IA32_EFER MSR (bit 10) is a status bit that indicates whether the logical processor is in IA-32e mode (and thus
     * uses either 4-level paging or 5-level paging). The processor always sets IA32_EFER.LMA to CR0.PG & IA32_EFER.LME. Software
     * cannot directly modify IA32_EFER.LMA; an execution of WRMSR to the IA32_EFER MSR ignores bit 10 of its source operand.
     */
    u8 efer_LMA() const { return cr0().c.PG & m_efer.c.LME; }

    MMU& mmu() { return m_mmu; }
    PIC& pic() { return m_pic; }
    ICU& icu() { return m_icu; }
    UARTController& uart1() { return m_uart1; };

private:
    void fix_interrupt_ext_bit(Interrupt& i) const;
    /**
     * Use this function only for sending interrupts that are processor-generated like exceptions
     * or software-generated interrupts (f.e. INT n instruction, etc.)
     * The error_code.standard.ext field will be updated if it was previously 0, to...
     *  - 1, if we are currently handling an interrupt i and i.source.is_external() == 1,
     *  - 0, otherwise
     */
    template<typename... Args>
    _InterruptRaised raise_interrupt(Interrupt i, Args&&... args) {
        fix_interrupt_ext_bit(i);

        // If we are handling the instruction, the interrupt is always integral part of instruction execution
        if (m_state == STATE_HANDLE_INSTRUCTION)
            return m_icu.raise_integral_interrupt(i);
        if (m_state == STATE_FETCH_INSTRUCTION && (i.vector == Exceptions::VEC_GP || i.vector == Exceptions::VEC_PF))
            return m_icu.raise_interrupt(i, std::forward<Args>(args)...);

        // Integral Exceptions are: DE, BP, OF, BR, TS, NP, SS, (GP), (PF), AC, MF, XM, VE, CP,
        switch (i.vector) {
            case Exceptions::VEC_DE:
            case Exceptions::VEC_BP:
            case Exceptions::VEC_OF:
            case Exceptions::VEC_BR:
            case Exceptions::VEC_TS:
            case Exceptions::VEC_NP:
            case Exceptions::VEC_SS:
            case Exceptions::VEC_AC:
            case Exceptions::VEC_MF:
            case Exceptions::VEC_XM:
            case Exceptions::VEC_VE:
            case Exceptions::VEC_CP: return m_icu.raise_integral_interrupt(i);
            default: return m_icu.raise_interrupt(i, std::forward<Args>(args)...);
        }
    }
    _InterruptRaised raise_integral_interrupt(Interrupt i);
    [[nodiscard]] InterruptRaisedOr<void> handle_nested_interrupt(Interrupt i);
    [[nodiscard]] InterruptRaisedOr<void> handle_interrupt(Interrupt i);
    // NOTE: as InterruptGateDescriptor and TrapGateDescriptor have the same layout, we simply choose one to receive
    [[nodiscard]] InterruptRaisedOr<void> enter_interrupt_trap_gate(Interrupt const& i, TrapGateDescriptor const& descriptor);
    [[nodiscard]] InterruptRaisedOr<void> enter_task_gate(Interrupt const& i, TaskGateDescriptor const& task_gate_descriptor);
    [[nodiscard]] InterruptRaisedOr<void> enter_call_gate(SegmentSelector const& selector, CallGateDescriptor const& call_gate_descriptor, bool through_call_insn);
    [[nodiscard]] InterruptRaisedOr<std::pair<SegmentSelector, u64>> do_stack_switch(u8 target_pl);

    [[nodiscard]] bool is_alignment_check_enabled() const { return cr0().c.AM && m_rflags.c.AC && cpl() == 3; }
    [[nodiscard]] InterruptRaisedOr<void> do_canonicality_check(VirtualAddress const& vaddr);

    DescriptorTable descriptor_table_of_selector(SegmentSelector selector) const;

    void update_rflags(ArithmeticResult res) {
        m_rflags.c.CF = res.has_cf_set;
        m_rflags.c.OF = res.has_of_set;
        update_rflags(res.value);
    }
    void update_rflags(SizedValue const& value) {
        m_rflags.c.SF = value.sign_bit();
        m_rflags.c.ZF = value.value() == 0x0;
        // TODO: PF
    }

    InterruptRaisedOr<LogicalAddress> logical_address(x86_op_mem const& mem) {
        auto seg = [&]() -> x86_reg {
            if (mem.segment != X86_REG_INVALID)
                return mem.segment;
            if (one_of(mem.base, {X86_REG_RSP, X86_REG_ESP, X86_REG_SP, X86_REG_RBP, X86_REG_EBP, X86_REG_BP}))
                return X86_REG_SS;
            return X86_REG_DS;
        }();
        u64 offset = MAY_HAVE_RAISED(operand_mem_offset(mem));
        return LogicalAddress{*application_segment_register(seg).value(), offset};
    }
    InterruptRaisedOr<VirtualAddress> virtual_address(x86_op_mem const& mem) {
        CPUE_ASSERT(mem.segment == X86_REG_INVALID, "Trying to interpret non-virtual address as virtual address.");
        return VirtualAddress{MAY_HAVE_RAISED(operand_mem_offset(mem))};
    }
    InterruptRaisedOr<u64> operand_mem_offset(x86_op_mem const& mem) {
        u64 offset = mem.base != X86_REG_INVALID ? MAY_HAVE_RAISED(reg(mem.base)->read()).value() : 0x0;
        if (!one_of(mem.index, {X86_REG_INVALID, X86_REG_RIZ, X86_REG_EIZ})) {
            offset += MAY_HAVE_RAISED(reg(mem.index)->read()).value() * mem.scale;
        }
        return offset + mem.disp;
    }

    std::optional<ApplicationSegmentRegister*> application_segment_register(x86_reg reg) {
        auto it = m_segment_register_alias_map.find(reg);
        if (it == m_segment_register_alias_map.end())
            return std::nullopt;
        return m_segment_register_map[it->second];
    }


private:
    [[nodiscard]] ByteWidth operand_byte_width(cs_x86_op const& operand) {
        switch (operand.type) {
            case X86_OP_REG: return reg(operand.reg)->byte_width();
            case X86_OP_MEM: return ByteWidth(operand.size);
            case X86_OP_IMM: return ByteWidth(operand.size);
            case X86_OP_INVALID: fail("operand_read on invalid operand.");
        }
    }
    [[nodiscard]] InterruptRaisedOr<SizedValue> operand_read(cs_x86_op const& operand);
    [[nodiscard]] InterruptRaisedOr<void> operand_write(cs_x86_op const& operand, SizedValue value);

    class Operand {
    public:
        Operand(CPU* cpu, cs_x86_op const& operand) : m_cpu(cpu), m_operand{operand} {}

        [[nodiscard]] InterruptRaisedOr<SizedValue> read() const { return m_cpu->operand_read(m_operand); }
        [[nodiscard]] InterruptRaisedOr<void> write(SizedValue const& value) const { return m_cpu->operand_write(m_operand, value); }

        ByteWidth byte_width() const { return m_cpu->operand_byte_width(m_operand); }

        cs_x86_op const& operand() const { return m_operand; }

    private:
        CPU* m_cpu;
        cs_x86_op const& m_operand;
    };



private:
    ICU m_icu;
    MMU m_mmu;
    PIC m_pic;
    Disassembler m_disassembler;

    UARTController m_uart1;

    std::optional<Interrupt> m_interrupt_to_be_handled;
    State m_state;

    // General Purpose / Pointer Registers
    u64 m_rax_val = 0x0;
    u64 m_rbx_val = 0x0;
    u64 m_rcx_val = 0x0;
    u64 m_rdx_val = 0x00000600;
    u64 m_rdi_val = 0x0;
    u64 m_rsi_val = 0x0;
    u64 m_rsp_val = 0x0;
    u64 m_rbp_val = 0x0;
    u64 m_r8_val = 0x0;
    u64 m_r9_val = 0x0;
    u64 m_r10_val = 0x0;
    u64 m_r11_val = 0x0;
    u64 m_r12_val = 0x0;
    u64 m_r13_val = 0x0;
    u64 m_r14_val = 0x0;
    u64 m_r15_val = 0x0;

    GeneralPurposeRegisterProxy m_rax = GeneralPurposeRegisterProxy::QWORD(&m_rax_val, this);
    GeneralPurposeRegisterProxy m_rbx = GeneralPurposeRegisterProxy::QWORD(&m_rbx_val, this);
    GeneralPurposeRegisterProxy m_rcx = GeneralPurposeRegisterProxy::QWORD(&m_rcx_val, this);
    GeneralPurposeRegisterProxy m_rdx = GeneralPurposeRegisterProxy::QWORD(&m_rdx_val, this);
    GeneralPurposeRegisterProxy m_rdi = GeneralPurposeRegisterProxy::QWORD(&m_rdi_val, this);
    GeneralPurposeRegisterProxy m_rsi = GeneralPurposeRegisterProxy::QWORD(&m_rsi_val, this);
    GeneralPurposeRegisterProxy m_rsp = GeneralPurposeRegisterProxy::QWORD(&m_rsp_val, this);
    GeneralPurposeRegisterProxy m_rbp = GeneralPurposeRegisterProxy::QWORD(&m_rbp_val, this);
    GeneralPurposeRegisterProxy m_r8 = GeneralPurposeRegisterProxy::QWORD(&m_r8_val, this);
    GeneralPurposeRegisterProxy m_r9 = GeneralPurposeRegisterProxy::QWORD(&m_r9_val, this);
    GeneralPurposeRegisterProxy m_r10 = GeneralPurposeRegisterProxy::QWORD(&m_r10_val, this);
    GeneralPurposeRegisterProxy m_r11 = GeneralPurposeRegisterProxy::QWORD(&m_r11_val, this);
    GeneralPurposeRegisterProxy m_r12 = GeneralPurposeRegisterProxy::QWORD(&m_r12_val, this);
    GeneralPurposeRegisterProxy m_r13 = GeneralPurposeRegisterProxy::QWORD(&m_r13_val, this);
    GeneralPurposeRegisterProxy m_r14 = GeneralPurposeRegisterProxy::QWORD(&m_r14_val, this);
    GeneralPurposeRegisterProxy m_r15 = GeneralPurposeRegisterProxy::QWORD(&m_r15_val, this);

    GeneralPurposeRegisterProxy m_eax = GeneralPurposeRegisterProxy::DWORD(&m_rax_val, this);
    GeneralPurposeRegisterProxy m_ebx = GeneralPurposeRegisterProxy::DWORD(&m_rbx_val, this);
    GeneralPurposeRegisterProxy m_ecx = GeneralPurposeRegisterProxy::DWORD(&m_rcx_val, this);
    GeneralPurposeRegisterProxy m_edx = GeneralPurposeRegisterProxy::DWORD(&m_rdx_val, this);
    GeneralPurposeRegisterProxy m_edi = GeneralPurposeRegisterProxy::DWORD(&m_rdi_val, this);
    GeneralPurposeRegisterProxy m_esi = GeneralPurposeRegisterProxy::DWORD(&m_rsi_val, this);
    GeneralPurposeRegisterProxy m_esp = GeneralPurposeRegisterProxy::DWORD(&m_rsp_val, this);
    GeneralPurposeRegisterProxy m_ebp = GeneralPurposeRegisterProxy::DWORD(&m_rbp_val, this);
    GeneralPurposeRegisterProxy m_r8d = GeneralPurposeRegisterProxy::DWORD(&m_r8_val, this);
    GeneralPurposeRegisterProxy m_r9d = GeneralPurposeRegisterProxy::DWORD(&m_r9_val, this);
    GeneralPurposeRegisterProxy m_r10d = GeneralPurposeRegisterProxy::DWORD(&m_r10_val, this);
    GeneralPurposeRegisterProxy m_r11d = GeneralPurposeRegisterProxy::DWORD(&m_r11_val, this);
    GeneralPurposeRegisterProxy m_r12d = GeneralPurposeRegisterProxy::DWORD(&m_r12_val, this);
    GeneralPurposeRegisterProxy m_r13d = GeneralPurposeRegisterProxy::DWORD(&m_r13_val, this);
    GeneralPurposeRegisterProxy m_r14d = GeneralPurposeRegisterProxy::DWORD(&m_r14_val, this);
    GeneralPurposeRegisterProxy m_r15d = GeneralPurposeRegisterProxy::DWORD(&m_r15_val, this);

    GeneralPurposeRegisterProxy m_ax = GeneralPurposeRegisterProxy::WORD(&m_rax_val, this);
    GeneralPurposeRegisterProxy m_bx = GeneralPurposeRegisterProxy::WORD(&m_rbx_val, this);
    GeneralPurposeRegisterProxy m_cx = GeneralPurposeRegisterProxy::WORD(&m_rcx_val, this);
    GeneralPurposeRegisterProxy m_dx = GeneralPurposeRegisterProxy::WORD(&m_rdx_val, this);
    GeneralPurposeRegisterProxy m_di = GeneralPurposeRegisterProxy::WORD(&m_rdi_val, this);
    GeneralPurposeRegisterProxy m_si = GeneralPurposeRegisterProxy::WORD(&m_rsi_val, this);
    GeneralPurposeRegisterProxy m_sp = GeneralPurposeRegisterProxy::WORD(&m_rsp_val, this);
    GeneralPurposeRegisterProxy m_bp = GeneralPurposeRegisterProxy::WORD(&m_rbp_val, this);
    GeneralPurposeRegisterProxy m_r8w = GeneralPurposeRegisterProxy::WORD(&m_r8_val, this);
    GeneralPurposeRegisterProxy m_r9w = GeneralPurposeRegisterProxy::WORD(&m_r9_val, this);
    GeneralPurposeRegisterProxy m_r10w = GeneralPurposeRegisterProxy::WORD(&m_r10_val, this);
    GeneralPurposeRegisterProxy m_r11w = GeneralPurposeRegisterProxy::WORD(&m_r11_val, this);
    GeneralPurposeRegisterProxy m_r12w = GeneralPurposeRegisterProxy::WORD(&m_r12_val, this);
    GeneralPurposeRegisterProxy m_r13w = GeneralPurposeRegisterProxy::WORD(&m_r13_val, this);
    GeneralPurposeRegisterProxy m_r14w = GeneralPurposeRegisterProxy::WORD(&m_r14_val, this);
    GeneralPurposeRegisterProxy m_r15w = GeneralPurposeRegisterProxy::WORD(&m_r15_val, this);

    GeneralPurposeRegisterProxy m_ah = GeneralPurposeRegisterProxy::HIGH(&m_rax_val, this);
    GeneralPurposeRegisterProxy m_bh = GeneralPurposeRegisterProxy::HIGH(&m_rbx_val, this);
    GeneralPurposeRegisterProxy m_ch = GeneralPurposeRegisterProxy::HIGH(&m_rcx_val, this);
    GeneralPurposeRegisterProxy m_dh = GeneralPurposeRegisterProxy::HIGH(&m_rdx_val, this);

    GeneralPurposeRegisterProxy m_al = GeneralPurposeRegisterProxy::LOW(&m_rax_val, this);
    GeneralPurposeRegisterProxy m_bl = GeneralPurposeRegisterProxy::LOW(&m_rbx_val, this);
    GeneralPurposeRegisterProxy m_cl = GeneralPurposeRegisterProxy::LOW(&m_rcx_val, this);
    GeneralPurposeRegisterProxy m_dl = GeneralPurposeRegisterProxy::LOW(&m_rdx_val, this);
    GeneralPurposeRegisterProxy m_dil = GeneralPurposeRegisterProxy::LOW(&m_rdi_val, this);
    GeneralPurposeRegisterProxy m_sil = GeneralPurposeRegisterProxy::LOW(&m_rsi_val, this);
    GeneralPurposeRegisterProxy m_spl = GeneralPurposeRegisterProxy::LOW(&m_rsp_val, this);
    GeneralPurposeRegisterProxy m_bpl = GeneralPurposeRegisterProxy::LOW(&m_rbp_val, this);
    GeneralPurposeRegisterProxy m_r8b = GeneralPurposeRegisterProxy::LOW(&m_r8_val, this);
    GeneralPurposeRegisterProxy m_r9b = GeneralPurposeRegisterProxy::LOW(&m_r9_val, this);
    GeneralPurposeRegisterProxy m_r10b = GeneralPurposeRegisterProxy::LOW(&m_r10_val, this);
    GeneralPurposeRegisterProxy m_r11b = GeneralPurposeRegisterProxy::LOW(&m_r11_val, this);
    GeneralPurposeRegisterProxy m_r12b = GeneralPurposeRegisterProxy::LOW(&m_r12_val, this);
    GeneralPurposeRegisterProxy m_r13b = GeneralPurposeRegisterProxy::LOW(&m_r13_val, this);
    GeneralPurposeRegisterProxy m_r14b = GeneralPurposeRegisterProxy::LOW(&m_r14_val, this);
    GeneralPurposeRegisterProxy m_r15b = GeneralPurposeRegisterProxy::LOW(&m_r15_val, this);

    u64 m_rip_val = 0x0000FFF0; // Only readable
    GeneralPurposeRegisterProxy m_rip = GeneralPurposeRegisterProxy::QWORD(&m_rip_val, this, REG_ACCESS_READ);
    GeneralPurposeRegisterProxy m_eip = GeneralPurposeRegisterProxy::DWORD(&m_rip_val, this, REG_ACCESS_READ);
    GeneralPurposeRegisterProxy m_ip = GeneralPurposeRegisterProxy::WORD(&m_rip_val, this, REG_ACCESS_READ);

    // IP of next instruction to be handled, provided for instruction handlers that need to know this
    u64 m_next_insn_rip;



    /**
     * See page 3174
     * Because ES, DS, and SS segment registers are not used in 64-bit mode, their fields (base, limit, and attribute) in
     * segment descriptor registers are ignored. Some forms of segment load instructions are also invalid (for example,
     * LDS, POP ES). Address calculations that reference the ES, DS, or SS segments are treated as if the segment base
     * is zero.
     *
     * The hidden descriptor register fields for FS.base and GS.base are physically mapped to MSRs in order to load all
     * address bits supported by a 64-bit implementation. Software with CPL = 0 (privileged software) can load all
     * supported linear-address bits into FS.base or GS.base using WRMSR. Addresses written into the 64-bit FS.base
     * and GS.base registers must be in canonical form. A WRMSR instruction that attempts to write a non-canonical
     * address to those registers causes a #GP fault.
     */
    ApplicationSegmentRegister m_cs;
    ApplicationSegmentRegister m_ds;
    ApplicationSegmentRegister m_ss;
    ApplicationSegmentRegister m_es;
    ApplicationSegmentRegister m_fs;
    ApplicationSegmentRegister m_gs;
    ApplicationSegmentRegister* m_segment_register_map[8] = {
        &m_cs, &m_ds, &m_ss, &m_es, &m_fs, &m_gs,
        NULL, // ldtr
        NULL, // tr
    };
    std::unordered_map<x86_reg, SegmentRegisterAlias> m_segment_register_alias_map = {
        {X86_REG_CS, SegmentRegisterAlias::CS},
        {X86_REG_DS, SegmentRegisterAlias::DS},
        {X86_REG_SS, SegmentRegisterAlias::SS},
        {X86_REG_ES, SegmentRegisterAlias::ES},
        {X86_REG_GS, SegmentRegisterAlias::GS},
        {X86_REG_FS, SegmentRegisterAlias::FS},
    };
    ApplicationSegmentRegisterProxy m_cs_reg = ApplicationSegmentRegisterProxy(&m_cs, SegmentRegisterAlias::CS, this);
    ApplicationSegmentRegisterProxy m_ds_reg = ApplicationSegmentRegisterProxy(&m_ds, SegmentRegisterAlias::DS, this);
    ApplicationSegmentRegisterProxy m_ss_reg = ApplicationSegmentRegisterProxy(&m_ss, SegmentRegisterAlias::SS, this);
    ApplicationSegmentRegisterProxy m_es_reg = ApplicationSegmentRegisterProxy(&m_es, SegmentRegisterAlias::ES, this);
    ApplicationSegmentRegisterProxy m_fs_reg = ApplicationSegmentRegisterProxy(&m_fs, SegmentRegisterAlias::FS, this);
    ApplicationSegmentRegisterProxy m_gs_reg = ApplicationSegmentRegisterProxy(&m_gs, SegmentRegisterAlias::GS, this);


    // RFLAGS
    RFLAGS m_rflags;

    /**
     * Control Registers.
     * CR1, CR5-7, CR9-15:
     * Reserved, the cpu will throw a #ud exception when trying to access them.
     */
    u64 m_cr0_val = 0x0;
    u64 m_cr1_val = 0x0;
    // // This control register contains the linear (virtual) address which triggered a page fault, available in the page fault's interrupt handler.
    u64 m_cr2_val = 0x0;
    u64 m_cr3_val = 0x0;
    u64 m_cr4_val = 0x0;
    u64 m_cr5_val = 0x0;
    u64 m_cr6_val = 0x0;
    u64 m_cr7_val = 0x0;
    u64 m_cr8_val = 0x0;
    u64 m_cr9_val = 0x0;
    u64 m_cr10_val = 0x0;
    u64 m_cr11_val = 0x0;
    u64 m_cr12_val = 0x0;
    u64 m_cr13_val = 0x0;
    u64 m_cr14_val = 0x0;
    u64 m_cr15_val = 0x0;

    u64 _old_cr0_val = 0x0;
    ControlRegisterProxy m_cr0_reg = ControlRegisterProxy(&m_cr0_val, this, REG_ACCESS_READ | REG_ACCESS_WRITE,
        RegisterCallbacks{
            .before_write = [](void* data) -> InterruptRaisedOr<void> {
                auto self = static_cast<CPU*>(data);
                self->_old_cr0_val = self->m_cr0_val;
                return {};
            },
            .after_write = [](void* data) -> InterruptRaisedOr<void> {
                // Invalidate TLB if PG bit was changed.
                auto self = static_cast<CPU*>(data);
                auto xored_cr0_val = self->m_cr0_val ^ self->_old_cr0_val;
                if (CR0{.value = xored_cr0_val}.c.PG)
                    self->m_mmu.tlb().invalidate_all();
                return {};
            },
            .data = this,
        });
    ControlRegisterProxy m_cr1_reg = ControlRegisterProxy(&m_cr1_val, this, REG_ACCESS_NONE);
    ControlRegisterProxy m_cr2_reg = ControlRegisterProxy(&m_cr2_val, this);
    ControlRegisterProxy m_cr3_reg = ControlRegisterProxy(&m_cr3_val, this, REG_ACCESS_READ | REG_ACCESS_WRITE,
        RegisterCallbacks{
            .after_write = [](void* data) -> InterruptRaisedOr<void> {
                auto tlb = static_cast<TLB*>(data);
                tlb->invalidate_all();
                return {};
            },
            .data = &m_mmu.tlb(),
        });
    ControlRegisterProxy m_cr4_reg = ControlRegisterProxy(&m_cr4_val, this);
    ControlRegisterProxy m_cr5_reg = ControlRegisterProxy(&m_cr5_val, this, REG_ACCESS_NONE);
    ControlRegisterProxy m_cr6_reg = ControlRegisterProxy(&m_cr6_val, this, REG_ACCESS_NONE);
    ControlRegisterProxy m_cr7_reg = ControlRegisterProxy(&m_cr7_val, this, REG_ACCESS_NONE);
    ControlRegisterProxy m_cr8_reg = ControlRegisterProxy(&m_cr8_val, this);
    ControlRegisterProxy m_cr9_reg = ControlRegisterProxy(&m_cr9_val, this, REG_ACCESS_NONE);
    ControlRegisterProxy m_cr10_reg = ControlRegisterProxy(&m_cr10_val, this, REG_ACCESS_NONE);
    ControlRegisterProxy m_cr11_reg = ControlRegisterProxy(&m_cr11_val, this, REG_ACCESS_NONE);
    ControlRegisterProxy m_cr12_reg = ControlRegisterProxy(&m_cr12_val, this, REG_ACCESS_NONE);
    ControlRegisterProxy m_cr13_reg = ControlRegisterProxy(&m_cr13_val, this, REG_ACCESS_NONE);
    ControlRegisterProxy m_cr14_reg = ControlRegisterProxy(&m_cr14_val, this, REG_ACCESS_NONE);
    ControlRegisterProxy m_cr15_reg = ControlRegisterProxy(&m_cr15_val, this, REG_ACCESS_NONE);


    /**
     * MSRs (Model-Specific-Registers)
     */
    EFER m_efer = {.value = 0x0};

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
    DescriptorTable m_gdtr;
    SystemSegmentRegister m_ldtr;
    DescriptorTable m_idtr;
    SystemSegmentRegister m_tr;

    SystemSegmentRegister* m_system_segment_register_map[8] = {
        NULL, // cs
        NULL, // ds
        NULL, // ss
        NULL, // es
        NULL, // fs
        NULL, // gs
        &m_ldtr,
        &m_tr,
    };


    RegisterProxy* m_register_map[247] = {
        nullptr, &m_ah, &m_al, &m_ax, &m_bh, &m_bl, &m_bp, &m_bpl, &m_bx, &m_ch, &m_cl, &m_cs_reg, &m_cx, &m_dh, &m_di, &m_dil, &m_dl, &m_ds_reg, &m_dx, &m_eax, &m_ebp, &m_ebx,
        &m_ecx, &m_edi, &m_edx, nullptr, &m_eip, nullptr, &m_es_reg, &m_esi, &m_esp, nullptr, &m_fs_reg, &m_gs_reg, &m_ip, &m_rax, &m_rbp, &m_rbx, &m_rcx, &m_rdi, &m_rdx, &m_rip,
        nullptr, &m_rsi, &m_rsp, &m_si, &m_sil, &m_sp, &m_spl, &m_ss_reg, &m_cr0_reg, &m_cr1_reg, &m_cr2_reg, &m_cr3_reg, &m_cr4_reg, &m_cr5_reg, &m_cr6_reg, &m_cr7_reg, &m_cr8_reg,
        &m_cr9_reg, &m_cr10_reg, &m_cr11_reg, &m_cr12_reg, &m_cr13_reg, &m_cr14_reg, &m_cr15_reg, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &m_r8, &m_r9, &m_r10, &m_r11, &m_r12, &m_r13, &m_r14, &m_r15,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &m_r8b, &m_r9b, &m_r10b, &m_r11b, &m_r12b, &m_r13b, &m_r14b, &m_r15b, &m_r8d, &m_r9d,
        &m_r10d, &m_r11d, &m_r12d, &m_r13d, &m_r14d, &m_r15d, &m_r8w, &m_r9w, &m_r10w, &m_r11w, &m_r12w, &m_r13w, &m_r14w, &m_r15w, nullptr, nullptr, nullptr, nullptr,
        nullptr, // <-- mark the end of the list of registers
    };

private:
    u64 next_insn_rip() const { return m_next_insn_rip; }
    [[nodiscard]] InterruptRaisedOr<void> do_privileged_instruction_check(u8 pl = 0);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_ADD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_CLI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_CMP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_DEC(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_DIV(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_HLT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_IDIV(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_IMUL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_INC(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_INT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_INT1(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_INT3(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_INTO(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_INVLPG(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_IRET(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_IRETD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_IRETQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_JMP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_JNE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_JE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_JGE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_JG(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_JLE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_JL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_LEA(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_LEAVE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_LGDT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_LIDT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_LLDT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_LTR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_LOOP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_LOOPE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_LOOPNE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_MOV(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_MOVABS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_MOVSX(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_MOVSXD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_MOVZX(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_MUL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_NOP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_NOT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_OR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_POP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_POPF(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_POPFQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_PUSH(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_PUSHF(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_PUSHFQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_RET(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_ROL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_ROR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_SAL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_SAR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_SGDT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_SHL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_SHLD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_SHLX(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_SHR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_SIDT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_SLDT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_STI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_SUB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_SWAPGS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_TEST(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_XCHG(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<IPIncrementBehavior> handle_XOR(cs_x86 const&);
};

}
