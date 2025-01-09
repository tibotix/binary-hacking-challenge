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
#include "register.h"


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

    enum PagingMode { PAGING_MODE_NONE, PAGING_MODE_32BIT, PAGING_MODE_PAE, PAGING_MODE_4LEVEL, PAGING_MODE_5LEVEL };

    struct RFLAGS {
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
        printf("Shutting down...");
        exit(0);
    }
    [[nodiscard]] InterruptRaisedOr<void> handle_insn(cs_insn const&);

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
        if (m_cr4.c.PCIDE == 0)
            return 0;
        return m_cr3.pcid();
    }

    u64 rip() const { return m_rip; }




    // TODO: also add them to register lookup table
    void set_cr0(u64 value) { m_cr0.value = value; }
    void set_cr2(u64 value) { m_cr2.addr = value; }
    void set_cr3(u64 value) { m_cr3.value = value; }
    void set_cr4(u64 value) { m_cr4.value = value; }
    void set_cr8(u64 value) { m_cr8.value = value; }


    LogicalAddress stack_pointer() const { return {m_ss, m_rsp.read()}; }
    [[nodiscard]] InterruptRaisedOr<void> stack_push(u64 value);
    [[nodiscard]] InterruptRaisedOr<u64> stack_pop();

    LogicalAddress code_pointer() const { return {m_cs, m_rip}; }


    [[nodiscard]] InterruptRaisedOr<void> load_segment_register(SegmentRegisterAlias alias, SegmentSelector selector);
    [[nodiscard]] InterruptRaisedOr<void> load_segment_register(SegmentRegisterAlias alias, SegmentSelector selector, GDTLDTDescriptor const& descriptor);


    template<typename V>
    std::optional<Register<V>*> gpreg(x86_reg reg) {
        auto gpreg_map = get_gpreg_map<V>();
        auto it = gpreg_map.find(reg);
        if (it == gpreg_map.end())
            return std::nullopt;
        return it->second;
    }
    std::optional<Register<u64>*> gpreg64(x86_reg reg) { return gpreg<u64>(reg); }
    std::optional<Register<u32>*> gpreg32(x86_reg reg) { return gpreg<u32>(reg); }
    std::optional<Register<u16>*> gpreg16(x86_reg reg) { return gpreg<u16>(reg); }
    std::optional<Register<u8>*> gpreg8(x86_reg reg) { return gpreg<u8>(reg); }



    RFLAGS& rflags() { return m_rflags; }
    EFER& efer() { return m_efer; }
    /**
     * The LMA flag in the IA32_EFER MSR (bit 10) is a status bit that indicates whether the logical processor is in IA-32e mode (and thus
     * uses either 4-level paging or 5-level paging). The processor always sets IA32_EFER.LMA to CR0.PG & IA32_EFER.LME. Software
     * cannot directly modify IA32_EFER.LMA; an execution of WRMSR to the IA32_EFER MSR ignores bit 10 of its source operand.
     */
    u8 efer_LMA() const { return m_cr0.c.PG & m_efer.c.LME; }

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

    [[nodiscard]] bool is_alignment_check_enabled() const { return m_cr0.c.AM && m_rflags.AC && cpl() == 3; }
    [[nodiscard]] InterruptRaisedOr<void> do_canonicality_check(VirtualAddress const& vaddr);

    DescriptorTable descriptor_table_of_selector(SegmentSelector selector) const;

    template<typename R>
    void update_rflags(ArithmeticResult<R> res) {
        m_rflags.CF = res.has_cf_set;
        m_rflags.OF = res.has_of_set;
        m_rflags.SF = res.has_sf_set;
        m_rflags.ZF = res.has_zf_set;
    }

    LogicalAddress logical_address(x86_op_mem const& mem) {
        CPUE_ASSERT(mem.segment != X86_REG_INVALID, "Trying to interpret memory address with invalid segment as logical address.");
        u64 offset = mem.base + (mem.index * mem.scale) + mem.disp;
        return LogicalAddress{*application_segment_register(mem.segment).value(), offset};
    }
    VirtualAddress virtual_address(x86_op_mem const& mem) {
        CPUE_ASSERT(mem.segment == X86_REG_INVALID, "Trying to interpret non-virtual address as virtual address.");
        return mem.base + (mem.index * mem.scale) + mem.disp;
    }

    // TODO: Maybe make public
    std::optional<ApplicationSegmentRegister*> application_segment_register(x86_reg reg) {
        auto it = m_segment_register_alias_map.find(reg);
        if (it == m_segment_register_alias_map.end())
            return std::nullopt;
        return m_segment_register_map[it->second];
    }

    template<typename V>
    constexpr std::unordered_map<x86_reg, Register<V>*>& get_gpreg_map() {
        if constexpr (std::is_same_v<V, u64>) {
            return m_gpreg64_map;
        } else if constexpr (std::is_same_v<V, u32>) {
            return m_gpreg32_map;
        } else if constexpr (std::is_same_v<V, u16>) {
            return m_gpreg16_map;
        } else if constexpr (std::is_same_v<V, u8>) {
            return m_gpreg8_map;
        } else {
            static_assert(always_false<V>, "Unsupported register type");
        }
    }

private:
    template<unsigned_integral T>
    [[nodiscard]] InterruptRaisedOr<T> operand_read(cs_x86_op const& operand) {
        switch (operand.type) {
            case X86_OP_REG: {
                switch (get_register_type(operand.reg)) {
                    case GENERAL_PURPOSE_REGISTER: return gpreg<T>(operand.reg).value()->read();
                    case APPLICATION_SEGMENT_REGISTER: return application_segment_register(operand.reg).value()->visible.segment_selector.value;
                    case CONTROL_REGISTER: TODO("operand_read: CONTROL_REGISTER.");
                    default: fail("operand_read called with unsupported register type");
                }
            }
            case X86_OP_MEM: return m_mmu.mem_read<T>(logical_address(operand.mem));
            case X86_OP_IMM: return static_cast<T>(operand.imm);
            case X86_OP_INVALID: fail("operand_read on invalid operand.");
        }
    }
    template<unsigned_integral T>
    [[nodiscard]] InterruptRaisedOr<void> operand_write(cs_x86_op const& operand, T value) {
        switch (operand.type) {
            case X86_OP_REG: {
                switch (get_register_type(operand.reg)) {
                    case GENERAL_PURPOSE_REGISTER: gpreg<T>(operand.reg).value()->write(value); return {};
                    case APPLICATION_SEGMENT_REGISTER: return load_segment_register(m_segment_register_alias_map[operand.reg], value);
                    case CONTROL_REGISTER: TODO("operand_write: CONTROL_REGISTER.");
                    default: fail("operand_write called with unsupported register type");
                }
            }
            case X86_OP_MEM: return m_mmu.mem_write<T>(logical_address(operand.mem), value);
            case X86_OP_IMM:
            case X86_OP_INVALID: fail("operand_write on invalid operand.");
        }
    }

    template<unsigned_integral T>
    class Operand {
    public:
        Operand(CPU* cpu, cs_x86_op& operand) : m_cpu(cpu), m_operand{operand} {}

        [[nodiscard]] InterruptRaisedOr<T> read() const { return m_cpu->operand_read<T>(m_operand); }
        [[nodiscard]] InterruptRaisedOr<void> write(T value) const { return m_cpu->operand_write<T>(m_operand, value); }

        cs_x86_op const& operand() const { return m_operand; }

    private:
        CPU* m_cpu;
        cs_x86_op& m_operand;
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

    Register<u64> m_rax = RegisterFactory::QWORD(&m_rax_val);
    Register<u64> m_rbx = RegisterFactory::QWORD(&m_rbx_val);
    Register<u64> m_rcx = RegisterFactory::QWORD(&m_rcx_val);
    Register<u64> m_rdx = RegisterFactory::QWORD(&m_rdx_val);
    Register<u64> m_rdi = RegisterFactory::QWORD(&m_rdi_val);
    Register<u64> m_rsi = RegisterFactory::QWORD(&m_rsi_val);
    Register<u64> m_rsp = RegisterFactory::QWORD(&m_rsp_val);
    Register<u64> m_rbp = RegisterFactory::QWORD(&m_rbp_val);
    Register<u64> m_r8 = RegisterFactory::QWORD(&m_r8_val);
    Register<u64> m_r9 = RegisterFactory::QWORD(&m_r9_val);
    Register<u64> m_r10 = RegisterFactory::QWORD(&m_r10_val);
    Register<u64> m_r11 = RegisterFactory::QWORD(&m_r11_val);
    Register<u64> m_r12 = RegisterFactory::QWORD(&m_r12_val);
    Register<u64> m_r13 = RegisterFactory::QWORD(&m_r13_val);
    Register<u64> m_r14 = RegisterFactory::QWORD(&m_r14_val);
    Register<u64> m_r15 = RegisterFactory::QWORD(&m_r15_val);

    Register<u32> m_eax = RegisterFactory::DWORD(&m_rax_val);
    Register<u32> m_ebx = RegisterFactory::DWORD(&m_rbx_val);
    Register<u32> m_ecx = RegisterFactory::DWORD(&m_rcx_val);
    Register<u32> m_edx = RegisterFactory::DWORD(&m_rdx_val);
    Register<u32> m_edi = RegisterFactory::DWORD(&m_rdi_val);
    Register<u32> m_esi = RegisterFactory::DWORD(&m_rsi_val);
    Register<u32> m_esp = RegisterFactory::DWORD(&m_rsp_val);
    Register<u32> m_ebp = RegisterFactory::DWORD(&m_rbp_val);
    Register<u32> m_r8d = RegisterFactory::DWORD(&m_r8_val);
    Register<u32> m_r9d = RegisterFactory::DWORD(&m_r9_val);
    Register<u32> m_r10d = RegisterFactory::DWORD(&m_r10_val);
    Register<u32> m_r11d = RegisterFactory::DWORD(&m_r11_val);
    Register<u32> m_r12d = RegisterFactory::DWORD(&m_r12_val);
    Register<u32> m_r13d = RegisterFactory::DWORD(&m_r13_val);
    Register<u32> m_r14d = RegisterFactory::DWORD(&m_r14_val);
    Register<u32> m_r15d = RegisterFactory::DWORD(&m_r15_val);

    Register<u16> m_ax = RegisterFactory::WORD(&m_rax_val);
    Register<u16> m_bx = RegisterFactory::WORD(&m_rbx_val);
    Register<u16> m_cx = RegisterFactory::WORD(&m_rcx_val);
    Register<u16> m_dx = RegisterFactory::WORD(&m_rdx_val);
    Register<u16> m_di = RegisterFactory::WORD(&m_rdi_val);
    Register<u16> m_si = RegisterFactory::WORD(&m_rsi_val);
    Register<u16> m_sp = RegisterFactory::WORD(&m_rsp_val);
    Register<u16> m_bp = RegisterFactory::WORD(&m_rbp_val);
    Register<u16> m_r8w = RegisterFactory::WORD(&m_r8_val);
    Register<u16> m_r9w = RegisterFactory::WORD(&m_r9_val);
    Register<u16> m_r10w = RegisterFactory::WORD(&m_r10_val);
    Register<u16> m_r11w = RegisterFactory::WORD(&m_r11_val);
    Register<u16> m_r12w = RegisterFactory::WORD(&m_r12_val);
    Register<u16> m_r13w = RegisterFactory::WORD(&m_r13_val);
    Register<u16> m_r14w = RegisterFactory::WORD(&m_r14_val);
    Register<u16> m_r15w = RegisterFactory::WORD(&m_r15_val);

    Register<u8> m_ah = RegisterFactory::HIGH(&m_rax_val);
    Register<u8> m_bh = RegisterFactory::HIGH(&m_rbx_val);
    Register<u8> m_ch = RegisterFactory::HIGH(&m_rcx_val);
    Register<u8> m_dh = RegisterFactory::HIGH(&m_rdx_val);

    Register<u8> m_al = RegisterFactory::LOW(&m_rax_val);
    Register<u8> m_bl = RegisterFactory::LOW(&m_rbx_val);
    Register<u8> m_cl = RegisterFactory::LOW(&m_rcx_val);
    Register<u8> m_dl = RegisterFactory::LOW(&m_rdx_val);
    Register<u8> m_dil = RegisterFactory::LOW(&m_rdi_val);
    Register<u8> m_sil = RegisterFactory::LOW(&m_rsi_val);
    Register<u8> m_spl = RegisterFactory::LOW(&m_rsp_val);
    Register<u8> m_bpl = RegisterFactory::LOW(&m_rbp_val);
    Register<u8> m_r8b = RegisterFactory::LOW(&m_r8_val);
    Register<u8> m_r9b = RegisterFactory::LOW(&m_r9_val);
    Register<u8> m_r10b = RegisterFactory::LOW(&m_r10_val);
    Register<u8> m_r11b = RegisterFactory::LOW(&m_r11_val);
    Register<u8> m_r12b = RegisterFactory::LOW(&m_r12_val);
    Register<u8> m_r13b = RegisterFactory::LOW(&m_r13_val);
    Register<u8> m_r14b = RegisterFactory::LOW(&m_r14_val);
    Register<u8> m_r15b = RegisterFactory::LOW(&m_r15_val);

    u64 m_rip = 0x0000FFF0;

    // Look up table for general purpose registers
    std::unordered_map<x86_reg, Register<u64>*> m_gpreg64_map = {{X86_REG_RAX, &m_rax}, {X86_REG_RBX, &m_rbx}, {X86_REG_RCX, &m_rcx}, {X86_REG_RDX, &m_rdx}, {X86_REG_RDI, &m_rdi},
        {X86_REG_RSI, &m_rsi}, {X86_REG_RSP, &m_rsp}, {X86_REG_RBP, &m_rbp}, {X86_REG_R8, &m_r8}, {X86_REG_R9, &m_r9}, {X86_REG_R10, &m_r10}, {X86_REG_R11, &m_r11},
        {X86_REG_R12, &m_r12}, {X86_REG_R13, &m_r13}, {X86_REG_R14, &m_r14}, {X86_REG_R15, &m_r15}};
    std::unordered_map<x86_reg, Register<u32>*> m_gpreg32_map = {
        {X86_REG_EAX, &m_eax},
        {X86_REG_EBX, &m_ebx},
        {X86_REG_ECX, &m_ecx},
        {X86_REG_EDX, &m_edx},
        {X86_REG_EDI, &m_edi},
        {X86_REG_ESI, &m_esi},
        {X86_REG_ESP, &m_esp},
        {X86_REG_EBP, &m_ebp},
        {X86_REG_R8D, &m_r8d},
        {X86_REG_R9D, &m_r9d},
        {X86_REG_R10D, &m_r10d},
        {X86_REG_R11D, &m_r11d},
        {X86_REG_R12D, &m_r12d},
        {X86_REG_R13D, &m_r13d},
        {X86_REG_R14D, &m_r14d},
        {X86_REG_R15D, &m_r15d},
    };
    std::unordered_map<x86_reg, Register<u16>*> m_gpreg16_map = {
        {X86_REG_AX, &m_ax},
        {X86_REG_BX, &m_bx},
        {X86_REG_CX, &m_cx},
        {X86_REG_DX, &m_dx},
        {X86_REG_DI, &m_di},
        {X86_REG_SI, &m_si},
        {X86_REG_SP, &m_sp},
        {X86_REG_BP, &m_bp},
        {X86_REG_R8W, &m_r8w},
        {X86_REG_R9W, &m_r9w},
        {X86_REG_R10W, &m_r10w},
        {X86_REG_R11W, &m_r11w},
        {X86_REG_R12W, &m_r12w},
        {X86_REG_R13W, &m_r13w},
        {X86_REG_R14W, &m_r14w},
        {X86_REG_R15W, &m_r15w},
    };
    std::unordered_map<x86_reg, Register<u8>*> m_gpreg8_map = {{X86_REG_AH, &m_ah}, {X86_REG_AL, &m_al}, {X86_REG_BH, &m_bh}, {X86_REG_BL, &m_bl}, {X86_REG_CH, &m_ch},
        {X86_REG_CL, &m_cl}, {X86_REG_DH, &m_dh}, {X86_REG_DL, &m_dl}, {X86_REG_DIL, &m_dil}, {X86_REG_SIL, &m_sil}, {X86_REG_SPL, &m_spl}, {X86_REG_BPL, &m_bpl},
        {X86_REG_R8B, &m_r8b}, {X86_REG_R9B, &m_r9b}, {X86_REG_R10B, &m_r10b}, {X86_REG_R11B, &m_r11b}, {X86_REG_R12B, &m_r12b}, {X86_REG_R13B, &m_r13b}, {X86_REG_R14B, &m_r14b},
        {X86_REG_R15B, &m_r15b}};


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


    // RFLAGS
    RFLAGS m_rflags;

    // Control Registers
    u64 m_cr0_val = 0x0;
    u64 m_cr2_val = 0x0;
    u64 m_cr3_val = 0x0;
    u64 m_cr4_val = 0x0;
    u64 m_cr8_val = 0x0;

    // TODO: use GRegister
    CR0 m_cr0 = {.value = 0x0};
    // This control register contains the linear (virtual) address which triggered a page fault, available in the page fault's interrupt handler.
    VirtualAddress m_cr2;
    CR3 m_cr3 = {.value = 0x0};
    CR4 m_cr4 = {.value = 0x0};
    CR8 m_cr8 = {.value = 0x0};
    /**
     * CR1, CR5-7, CR9-15:
     * Reserved, the cpu will throw a #ud exception when trying to access them.
     */


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

private:
#define PARENS      ()
#define EXPAND(...) __VA_ARGS__

#define ARGS1() operand_a
#define ARGS2() operand_a, operand_aa
#define ARGS3() operand_a, operand_aa, operand_aaa

#define IIF(...) __VA_ARGS__
#define IIF0(...)
#define IF(c, ...) IIF##c(__VA_ARGS__)

#define DISPATCH_OPS_IMPL(count, cur, ...)            \
    IF(__VA_OPT__(0), DISPATCH_OPS_IMPL2(count, cur)) \
    __VA_OPT__(DISPATCH_OPS_IMPLX(count, cur, __VA_ARGS__))
#define DISPATCH_OPS_IMPL2(count, cur) return func(ARGS##count());
#define DISPATCH_OPS_IMPLX(count, cur, op, ...)                         \
    switch (op.size) {                                                  \
        case 1: {                                                       \
            auto operand_##cur = Operand<u8>(this, op);                 \
            DISPATCH_OPS_IMPL_AGAIN PARENS(count, cur##a, __VA_ARGS__); \
        };                                                              \
        case 2: {                                                       \
            auto operand_##cur = Operand<u16>(this, op);                \
            DISPATCH_OPS_IMPL_AGAIN PARENS(count, cur##a, __VA_ARGS__); \
        };                                                              \
        case 4: {                                                       \
            auto operand_##cur = Operand<u32>(this, op);                \
            DISPATCH_OPS_IMPL_AGAIN PARENS(count, cur##a, __VA_ARGS__); \
        };                                                              \
        case 8: {                                                       \
            auto operand_##cur = Operand<u64>(this, op);                \
            DISPATCH_OPS_IMPL_AGAIN PARENS(count, cur##a, __VA_ARGS__); \
        };                                                              \
    }
#define DISPATCH_OPS_IMPL_AGAIN()                    DISPATCH_OPS_IMPL
#define DISPATCH_OPS1(first_op)                      EXPAND(DISPATCH_OPS_IMPL(1, a, first_op))
#define DISPATCH_OPS2(first_op, second_op)           EXPAND(EXPAND(DISPATCH_OPS_IMPL(2, a, first_op, second_op)))
#define DISPATCH_OPS3(first_op, second_op, third_op) EXPAND(EXPAND(EXPAND(DISPATCH_OPS_IMPL(3, a, first_op, second_op, third_op))))
    template<typename Func>
    InterruptRaisedOr<void> with_operands1(cs_x86_op& first_op, Func&& func) {
        DISPATCH_OPS1(first_op);
        fail();
    }
    template<typename Func>
    InterruptRaisedOr<void> with_operands2(cs_x86_op& first_op, cs_x86_op& second_op, Func&& func) {
        DISPATCH_OPS2(first_op, second_op);
        fail();
    }
    template<typename Func>
    InterruptRaisedOr<void> with_operands3(cs_x86_op& first_op, cs_x86_op& second_op, cs_x86_op& third_op, Func&& func) {
        DISPATCH_OPS3(first_op, second_op, third_op);
        fail();
    }
#undef PARENS
#undef EXPAND
#undef ARGS1
#undef ARGS2
#undef ARGS3
#undef IIF
#undef IIF0
#undef IF
#undef DISPATCH_OPS_IMPL
#undef DISPATCH_OPS_IMPL2
#undef DISPATCH_OPS_IMPLX
#undef DISPATCH_OPS_IMPL_AGAIN
#undef DISPATCH_OPS1
#undef DISPATCH_OPS2
#undef DISPATCH_OPS3


private:
    [[nodiscard]] InterruptRaisedOr<void> handle_AAA(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_AAD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_AAM(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_AAS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ADC(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ADCX(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ADD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ADDPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ADDPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ADDSD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ADDSS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ADDSUBPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ADDSUBPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ADOX(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_AESDEC(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_AESDEC128KL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_AESDEC256KL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_AESDECLAST(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_AESDECWIDE128KL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_AESDECWIDE256KL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_AESENC(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_AESENC128KL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_AESENC256KL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_AESENCLAST(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_AESENCWIDE128KL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_AESENCWIDE256KL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_AESIMC(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_AESKEYGENASSIST(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_AND(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ANDN(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ANDNPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ANDNPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ANDPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ANDPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ARPL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BEXTR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BLENDPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BLENDPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BLENDVPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BLENDVPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BLSI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BLSMSK(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BLSR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BNDCL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BNDCN(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BNDCU(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BNDLDX(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BNDMK(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BNDMOV(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BNDSTX(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BOUND(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BSF(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BSR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BSWAP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BTC(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BTR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BTS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_BZHI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CALL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CBW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CDQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CDQE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CLAC(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CLC(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CLD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CLDEMOTE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CLFLUSH(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CLFLUSHOPT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CLI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CLRSSBSY(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CLTS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CLUI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CLWB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CMC(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CMOVcc(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CMP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPSB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPSD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPSQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPSS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPXCHG(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPXCHG16B(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPXCHG8B(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_COMISD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_COMISS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CPUID(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CQO(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CRC32(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTDQ2PD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTDQ2PS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTPD2DQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTPD2PI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTPD2PS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTPI2PD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTPI2PS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTPS2DQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTPS2PD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTPS2PI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTSD2SI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTSD2SS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTSI2SD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTSI2SS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTSS2SD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTSS2SI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTTPD2DQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTTPD2PI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTTPS2DQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTTPS2PI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTTSD2SI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTTSS2SI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CWD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_CWDE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_DAA(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_DAS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_DEC(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_DIV(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_DIVPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_DIVPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_DIVSD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_DIVSS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_DPPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_DPPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_EMMS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ENCODEKEY128(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ENCODEKEY256(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ENDBR32(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ENDBR64(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ENQCMD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ENQCMDS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ENTER(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_EXTRACTPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_F2XM1(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FABS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FADD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FADDP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FBLD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FBSTP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FCHS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FCLEX(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FCMOVcc(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FCOM(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FCOMI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FCOMIP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FCOMP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FCOMPP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FCOS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FDECSTP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FDIV(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FDIVP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FDIVR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FDIVRP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FFREE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FIADD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FICOM(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FICOMP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FIDIV(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FIDIVR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FILD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FIMUL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FINCSTP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FINIT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FIST(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FISTP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FISTTP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FISUB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FISUBR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FLD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FLD1(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FLDCW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FLDENV(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FLDL2E(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FLDL2T(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FLDLG2(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FLDLN2(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FLDPI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FLDZ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FMUL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FMULP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FNCLEX(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FNINIT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FNOP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FNSAVE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FNSTCW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FNSTENV(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FNSTSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FPATAN(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FPREM(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FPREM1(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FPTAN(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FRNDINT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FRSTOR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FSAVE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FSCALE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FSIN(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FSINCOS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FSQRT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FST(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FSTCW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FSTENV(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FSTP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FSTSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FSUB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FSUBP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FSUBR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FSUBRP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FTST(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FUCOM(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FUCOMI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FUCOMIP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FUCOMP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FUCOMPP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FWAIT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FXAM(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FXCH(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FXRSTOR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FXSAVE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FXTRACT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FYL2X(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_FYL2XP1(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_GF2P8AFFINEINVQB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_GF2P8AFFINEQB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_GF2P8MULB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_HADDPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_HADDPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_HLT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_HRESET(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_HSUBPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_HSUBPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_IDIV(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_IMUL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_IN(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_INC(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_INCSSPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_INCSSPQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_INS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_INSB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_INSD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_INSERTPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_INSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_INT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_INT1(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_INT3(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_INTO(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_INVD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_INVLPG(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_INVPCID(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_IRET(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_IRETD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_IRETQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_JMP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_Jcc(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KADDB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KADDD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KADDQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KADDW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KANDB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KANDD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KANDNB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KANDND(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KANDNQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KANDNW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KANDQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KANDW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KMOVB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KMOVD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KMOVQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KMOVW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KNOTB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KNOTD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KNOTQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KNOTW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KORB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KORD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KORQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KORTESTB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KORTESTD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KORTESTQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KORTESTW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KORW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KSHIFTLB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KSHIFTLD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KSHIFTLQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KSHIFTLW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KSHIFTRB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KSHIFTRD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KSHIFTRQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KSHIFTRW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KTESTB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KTESTD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KTESTQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KTESTW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KUNPCKBW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KUNPCKDQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KUNPCKWD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KXNORB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KXNORD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KXNORQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KXNORW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KXORB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KXORD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KXORQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_KXORW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LAHF(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LAR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LDDQU(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LDMXCSR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LDS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LDTILECFG(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LEA(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LEAVE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LES(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LFENCE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LFS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LGDT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LGS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LIDT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LLDT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LMSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LOADIWKEY(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LOCK(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LODS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LODSB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LODSD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LODSQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LODSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LOOP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LOOPcc(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LSL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LSS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LTR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_LZCNT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MASKMOVDQU(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MASKMOVQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MAXPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MAXPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MAXSD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MAXSS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MFENCE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MINPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MINPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MINSD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MINSS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MONITOR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOV(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVAPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVAPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVBE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVDDUP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVDIR64B(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVDIRI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVDQ2Q(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVDQA(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVDQU(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVHLPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVHPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVHPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVLHPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVLPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVLPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVMSKPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVMSKPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVNTDQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVNTDQA(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVNTI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVNTPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVNTPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVNTQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVQ2DQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVSB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVSD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVSHDUP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVSLDUP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVSQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVSS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVSX(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVSXD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVUPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVUPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVZX(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MPSADBW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MUL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MULPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MULPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MULSD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MULSS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MULX(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_MWAIT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_NEG(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_NOP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_NOT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_OR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ORPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ORPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_OUT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_OUTS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_OUTSB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_OUTSD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_OUTSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PABSB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PABSD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PABSQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PABSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PACKSSDW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PACKSSWB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PACKUSDW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PACKUSWB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PADDB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PADDD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PADDQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PADDSB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PADDSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PADDUSB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PADDUSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PADDW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PALIGNR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PAND(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PANDN(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PAUSE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PAVGB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PAVGW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PBLENDVB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PBLENDW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PCLMULQDQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPEQB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPEQD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPEQQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPEQW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPESTRI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPESTRM(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPGTB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPGTD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPGTQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPGTW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPISTRI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPISTRM(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PCONFIG(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PDEP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PEXT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PEXTRB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PEXTRD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PEXTRQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PEXTRW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PHADDD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PHADDSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PHADDW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PHMINPOSUW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PHSUBD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PHSUBSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PHSUBW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PINSRB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PINSRD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PINSRQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PINSRW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMADDUBSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMADDWD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMAXSB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMAXSD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMAXSQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMAXSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMAXUB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMAXUD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMAXUQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMAXUW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMINSB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMINSD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMINSQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMINSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMINUB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMINUD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMINUQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMINUW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMOVMSKB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMOVSX(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMOVZX(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMULDQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMULHRSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMULHUW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMULHW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMULLD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMULLQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMULLW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PMULUDQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_POP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_POPA(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_POPAD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_POPCNT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_POPF(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_POPFD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_POPFQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_POR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PREFETCHW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PREFETCHh(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSADBW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSHUFB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSHUFD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSHUFHW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSHUFLW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSHUFW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSIGNB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSIGND(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSIGNW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSLLD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSLLDQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSLLQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSLLW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSRAD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSRAQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSRAW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSRLD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSRLDQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSRLQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSRLW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSUBB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSUBD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSUBQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSUBSB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSUBSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSUBUSB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSUBUSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PSUBW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PTEST(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PTWRITE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PUNPCKHBW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PUNPCKHDQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PUNPCKHQDQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PUNPCKHWD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PUNPCKLBW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PUNPCKLDQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PUNPCKLQDQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PUNPCKLWD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PUSH(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PUSHA(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PUSHAD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PUSHF(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PUSHFD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PUSHFQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_PXOR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RCL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RCPPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RCPSS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RCR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RDFSBASE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RDGSBASE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RDMSR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RDPID(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RDPKRU(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RDPMC(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RDRAND(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RDSEED(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RDSSPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RDSSPQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RDTSC(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RDTSCP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_REP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_REPE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_REPNE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_REPNZ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_REPZ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RET(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ROL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ROR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RORX(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ROUNDPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ROUNDPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ROUNDSD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_ROUNDSS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RSM(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RSQRTPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RSQRTSS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_RSTORSSP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SAHF(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SAL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SAR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SARX(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SAVEPREVSSP(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SBB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SCAS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SCASB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SCASD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SCASW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SENDUIPI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SERIALIZE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SETSSBSY(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SETcc(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SFENCE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SGDT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SHA1MSG1(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SHA1MSG2(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SHA1NEXTE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SHA1RNDS4(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SHA256MSG1(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SHA256MSG2(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SHA256RNDS2(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SHL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SHLD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SHLX(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SHR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SHRD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SHRX(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SHUFPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SHUFPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SIDT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SLDT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SMSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SQRTPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SQRTPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SQRTSD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SQRTSS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_STAC(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_STC(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_STD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_STI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_STMXCSR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_STOS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_STOSB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_STOSD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_STOSQ(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_STOSW(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_STR(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_STTILECFG(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_STUI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SUB(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SUBPD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SUBPS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SUBSD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SUBSS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SWAPGS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SYSCALL(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SYSENTER(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SYSEXIT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_SYSRET(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_TDPBF16PS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_TDPBSSD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_TDPBSUD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_TDPBUSD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_TDPBUUD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_TEST(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_TESTUI(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_TILELOADD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_TILERELEASE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_TILESTORED(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_TILEZERO(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_TPAUSE(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_TZCNT(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_UCOMISD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_UCOMISS(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_UD(cs_x86 const&);
    [[nodiscard]] InterruptRaisedOr<void> handle_UIRET(cs_x86 const&);
};

}
