#pragma once

#include <stack>

#include "mmu.h"
#include "address.h"
#include "disassembler.h"
#include "pic.h"
#include "segmentation.h"
#include "forward.h"


namespace CPUE {

// TODO: when implementing protected instructions, see chapter 2.8: System Instruction Summary
// TODO: when implementing some instructions, look at type checking (chapter 6.4 or page 3250)



/**
 * Interrupt Controller Unit
 * This class encapsulates functionality related to interrupts and exceptions.
 * It acts as a centralized location to receive interrupts from:
 *  - the processor itself (processor-generated exceptions)
 *  - NMI pin
 *  - INTR pin
 */
class ICU {
public:
    friend class CPU;
    ICU() = default;
    ICU(ICU const&) = delete;

private:
    static constexpr u8 MAX_PENDING_CAPACITY = 127;
    _InterruptRaised raise_interrupt(Interrupt i, u8 priority) {
        CPUE_ASSERT(1 <= priority <= 9, "priority must be between 1 and 9");
        return _raise_interrupt(i, priority);
    }
    _InterruptRaised raise_interrupt(Interrupt i) {
        TODO_NOFAIL("Implement proper interrupt priority setting");
        u8 priority = [&]() -> u8 {
            switch (i.type) {
                case InterruptType::ABORT_EXCEPTION: return 1;
                case InterruptType::TRAP_EXCEPTION: return 2;
                case InterruptType::FAULT_EXCEPTION: return 7;
                case InterruptType::NON_MASKABLE_INTERRUPT: return 5;
                case InterruptType::MASKABLE_INTERRUPT: return 6;
                default: break;
            }
            return 9;
        }();
        return raise_interrupt(i, priority);
    }
    /**
     * Instruction integral interrupts are handled as an integral part of the current instruction,
     * so they get a virtual highest priority.
     * NOTE: If #PF and #GP occur during fetching of an instruction, they are not integral.
     *       If #GP occur during decoding of an instruction (cause Instruction length > 15 bytes), they are not integral.
     *       Otherwise they are integral.
     * Integral Exceptions are: DE, BP, OF, BR, TS, NP, SS, GP, PF, AC, MF, XM, VE, CP,
     */
    _InterruptRaised raise_integral_interrupt(Interrupt i) { return _raise_interrupt(i, 0); }
    _InterruptRaised nmi_raise_interrupt(Interrupt i) { fail("NMI pin is not implemented"); }

    std::optional<Interrupt> pop_highest_priority_interrupt() {
        if (m_pending_interrupts.empty())
            return {};
        auto prioritized_interrupt = m_pending_interrupts.top();
        m_pending_interrupts.pop();
        return prioritized_interrupt.interrupt;
    }

private:
    _InterruptRaised _raise_interrupt(Interrupt i, u8 priority) {
        CPUE_ASSERT(m_pending_interrupts.size() < MAX_PENDING_CAPACITY, "ICU capacity full");
        m_pending_interrupts.push({i, priority});
        return INTERRUPT_RAISED;
    }

    struct _PrioritizedInterrupt {
        Interrupt interrupt;
        u8 priority;
    };
    struct {
        bool operator()(_PrioritizedInterrupt const l, _PrioritizedInterrupt const r) const { return l.priority < r.priority; }
    } _comparator;
    std::priority_queue<_PrioritizedInterrupt, std::vector<_PrioritizedInterrupt>, decltype(_comparator)> m_pending_interrupts{_comparator};
};


class CPU {
public:
    friend MMU;
    friend Disassembler;
    friend TLB;
    CPU() : m_mmu{this, 2}, m_disassembler(this) { reset(); }
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


public:
    [[nodiscard]] bool is_paging_enabled() const {
        TODO_NOFAIL("is_paging_enabled: use actual registers");
        return true;
    }
    void assert_paging_enabled() const { CPUE_ASSERT(is_paging_enabled(), "paging not enabled"); }
    [[nodiscard]] ExecutionMode execution_mode() const {
        if (m_efer.LMA == 1)
            return ExecutionMode::LONG_MODE;
        TODO_NOFAIL("execution_mode");
        return ExecutionMode::COMPATIBILITY_MODE;
    }
    void assert_in_long_mode() const { CPUE_ASSERT(execution_mode() == ExecutionMode::LONG_MODE, "not in long mode"); }

    [[nodiscard]] State state() const { return m_state; }

    void interpreter_loop();
    [[noreturn]] void shutdown() {
        printf("Shutting down...");
        exit(0);
    }

    void reset();

    [[nodiscard]] u8 cpl() const { return m_cs.visible.segment_selector.rpl; }
    [[nodiscard]] PrivilegeMode cpm() const { return cpl() == 3 ? USER_MODE : SUPERVISOR_MODE; }
    void set_cpl(u8 cpl) { m_cs.visible.segment_selector.rpl = cpl; }

    /**
     * Process-Context Identifiers (PCIDs) (See page 3230)
     * A PCID is a 12-bit identifier. Non-zero PCIDs are enabled by setting the PCIDE flag (bit 17) of CR4. If CR4.PCIDE =
     * 0, the current PCID is always 000H; otherwise, the current PCID is the value of bits 11:0 of CR3.1 Not all proces-
     * sors allow CR4.PCIDE to be set to 1;
     */
    [[nodiscard]] PCID pcid() const {
        if (m_cr4.PCIDE == 0)
            return 0;
        return m_cr3.pcid();
    }


    LogicalAddress stack_pointer() const { return {m_ss, m_rsp}; }
    InterruptRaisedOr<void> stack_push(u64 value);
    InterruptRaisedOr<u64> stack_pop();


    InterruptRaisedOr<void> load_segment_register(SegmentRegisterAlias alias, SegmentSelector selector);
    InterruptRaisedOr<void> load_segment_register(SegmentRegisterAlias alias, SegmentSelector selector, GDTLDTDescriptor const& descriptor);

    MMU& mmu() { return m_mmu; }
    PIC& pic() { return m_pic; }
    ICU& icu() { return m_icu; }

private:
    /**
     * Use this function only for sending interrupts that are processor-generated like exceptions
     * or software-generated interrupts (f.e. INT n instruction, etc.)
     * The error_code.standard.ext field will be updated if it was previously 0, to...
     *  - 1, if we are currently handling an interrupt i and i.source.is_external() == 1,
     *  - 0, otherwise
     */
    template<typename... Args>
    _InterruptRaised raise_interrupt(Interrupt i, Args&&... args);
    _InterruptRaised raise_integral_interrupt(Interrupt i) { return m_icu.raise_integral_interrupt(i); }
    InterruptRaisedOr<void> handle_nested_interrupt(Interrupt i);
    InterruptRaisedOr<void> handle_interrupt(Interrupt i);
    // NOTE: as InterruptGateDescriptor and TrapGateDescriptor have the same layout, we simply choose one to receive
    InterruptRaisedOr<void> enter_interrupt_trap_gate(Interrupt const& i, TrapGateDescriptor const& descriptor);
    InterruptRaisedOr<void> enter_task_gate(Interrupt const& i, TaskGateDescriptor const& task_gate_descriptor);
    InterruptRaisedOr<void> enter_call_gate(SegmentSelector const& selector, CallGateDescriptor const& call_gate_descriptor, bool through_call_insn);
    InterruptRaisedOr<std::pair<SegmentSelector, u64>> do_stack_switch(u8 target_pl);

    [[nodiscard]] bool alignment_check_enabled() const { return m_cr0.AM && m_rflags.AC && cpl() == 3; }
    InterruptRaisedOr<void> do_canonicality_check(VirtualAddress const& vaddr);

    DescriptorTable descriptor_table_of_selector(SegmentSelector selector) const;

private:
    InterruptRaisedOr<void> handle_AAA();
    InterruptRaisedOr<void> handle_AAD();
    InterruptRaisedOr<void> handle_AAM();
    InterruptRaisedOr<void> handle_AAS();
    InterruptRaisedOr<void> handle_ADC();
    InterruptRaisedOr<void> handle_ADCX();
    InterruptRaisedOr<void> handle_ADD();
    InterruptRaisedOr<void> handle_ADDPD();
    InterruptRaisedOr<void> handle_ADDPS();
    InterruptRaisedOr<void> handle_ADDSD();
    InterruptRaisedOr<void> handle_ADDSS();
    InterruptRaisedOr<void> handle_ADDSUBPD();
    InterruptRaisedOr<void> handle_ADDSUBPS();
    InterruptRaisedOr<void> handle_ADOX();
    InterruptRaisedOr<void> handle_AESDEC();
    InterruptRaisedOr<void> handle_AESDEC128KL();
    InterruptRaisedOr<void> handle_AESDEC256KL();
    InterruptRaisedOr<void> handle_AESDECLAST();
    InterruptRaisedOr<void> handle_AESDECWIDE128KL();
    InterruptRaisedOr<void> handle_AESDECWIDE256KL();
    InterruptRaisedOr<void> handle_AESENC();
    InterruptRaisedOr<void> handle_AESENC128KL();
    InterruptRaisedOr<void> handle_AESENC256KL();
    InterruptRaisedOr<void> handle_AESENCLAST();
    InterruptRaisedOr<void> handle_AESENCWIDE128KL();
    InterruptRaisedOr<void> handle_AESENCWIDE256KL();
    InterruptRaisedOr<void> handle_AESIMC();
    InterruptRaisedOr<void> handle_AESKEYGENASSIST();
    InterruptRaisedOr<void> handle_AND();
    InterruptRaisedOr<void> handle_ANDN();
    InterruptRaisedOr<void> handle_ANDNPD();
    InterruptRaisedOr<void> handle_ANDNPS();
    InterruptRaisedOr<void> handle_ANDPD();
    InterruptRaisedOr<void> handle_ANDPS();
    InterruptRaisedOr<void> handle_ARPL();
    InterruptRaisedOr<void> handle_BEXTR();
    InterruptRaisedOr<void> handle_BLENDPD();
    InterruptRaisedOr<void> handle_BLENDPS();
    InterruptRaisedOr<void> handle_BLENDVPD();
    InterruptRaisedOr<void> handle_BLENDVPS();
    InterruptRaisedOr<void> handle_BLSI();
    InterruptRaisedOr<void> handle_BLSMSK();
    InterruptRaisedOr<void> handle_BLSR();
    InterruptRaisedOr<void> handle_BNDCL();
    InterruptRaisedOr<void> handle_BNDCN();
    InterruptRaisedOr<void> handle_BNDCU();
    InterruptRaisedOr<void> handle_BNDLDX();
    InterruptRaisedOr<void> handle_BNDMK();
    InterruptRaisedOr<void> handle_BNDMOV();
    InterruptRaisedOr<void> handle_BNDSTX();
    InterruptRaisedOr<void> handle_BOUND();
    InterruptRaisedOr<void> handle_BSF();
    InterruptRaisedOr<void> handle_BSR();
    InterruptRaisedOr<void> handle_BSWAP();
    InterruptRaisedOr<void> handle_BT();
    InterruptRaisedOr<void> handle_BTC();
    InterruptRaisedOr<void> handle_BTR();
    InterruptRaisedOr<void> handle_BTS();
    InterruptRaisedOr<void> handle_BZHI();
    InterruptRaisedOr<void> handle_CALL();
    InterruptRaisedOr<void> handle_CBW();
    InterruptRaisedOr<void> handle_CDQ();
    InterruptRaisedOr<void> handle_CDQE();
    InterruptRaisedOr<void> handle_CLAC();
    InterruptRaisedOr<void> handle_CLC();
    InterruptRaisedOr<void> handle_CLD();
    InterruptRaisedOr<void> handle_CLDEMOTE();
    InterruptRaisedOr<void> handle_CLFLUSH();
    InterruptRaisedOr<void> handle_CLFLUSHOPT();
    InterruptRaisedOr<void> handle_CLI();
    InterruptRaisedOr<void> handle_CLRSSBSY();
    InterruptRaisedOr<void> handle_CLTS();
    InterruptRaisedOr<void> handle_CLUI();
    InterruptRaisedOr<void> handle_CLWB();
    InterruptRaisedOr<void> handle_CMC();
    InterruptRaisedOr<void> handle_CMOVcc();
    InterruptRaisedOr<void> handle_CMP();
    InterruptRaisedOr<void> handle_CMPPD();
    InterruptRaisedOr<void> handle_CMPPS();
    InterruptRaisedOr<void> handle_CMPS();
    InterruptRaisedOr<void> handle_CMPSB();
    InterruptRaisedOr<void> handle_CMPSD();
    InterruptRaisedOr<void> handle_CMPSQ();
    InterruptRaisedOr<void> handle_CMPSS();
    InterruptRaisedOr<void> handle_CMPSW();
    InterruptRaisedOr<void> handle_CMPXCHG();
    InterruptRaisedOr<void> handle_CMPXCHG16B();
    InterruptRaisedOr<void> handle_CMPXCHG8B();
    InterruptRaisedOr<void> handle_COMISD();
    InterruptRaisedOr<void> handle_COMISS();
    InterruptRaisedOr<void> handle_CPUID();
    InterruptRaisedOr<void> handle_CQO();
    InterruptRaisedOr<void> handle_CRC32();
    InterruptRaisedOr<void> handle_CVTDQ2PD();
    InterruptRaisedOr<void> handle_CVTDQ2PS();
    InterruptRaisedOr<void> handle_CVTPD2DQ();
    InterruptRaisedOr<void> handle_CVTPD2PI();
    InterruptRaisedOr<void> handle_CVTPD2PS();
    InterruptRaisedOr<void> handle_CVTPI2PD();
    InterruptRaisedOr<void> handle_CVTPI2PS();
    InterruptRaisedOr<void> handle_CVTPS2DQ();
    InterruptRaisedOr<void> handle_CVTPS2PD();
    InterruptRaisedOr<void> handle_CVTPS2PI();
    InterruptRaisedOr<void> handle_CVTSD2SI();
    InterruptRaisedOr<void> handle_CVTSD2SS();
    InterruptRaisedOr<void> handle_CVTSI2SD();
    InterruptRaisedOr<void> handle_CVTSI2SS();
    InterruptRaisedOr<void> handle_CVTSS2SD();
    InterruptRaisedOr<void> handle_CVTSS2SI();
    InterruptRaisedOr<void> handle_CVTTPD2DQ();
    InterruptRaisedOr<void> handle_CVTTPD2PI();
    InterruptRaisedOr<void> handle_CVTTPS2DQ();
    InterruptRaisedOr<void> handle_CVTTPS2PI();
    InterruptRaisedOr<void> handle_CVTTSD2SI();
    InterruptRaisedOr<void> handle_CVTTSS2SI();
    InterruptRaisedOr<void> handle_CWD();
    InterruptRaisedOr<void> handle_CWDE();
    InterruptRaisedOr<void> handle_DAA();
    InterruptRaisedOr<void> handle_DAS();
    InterruptRaisedOr<void> handle_DEC();
    InterruptRaisedOr<void> handle_DIV();
    InterruptRaisedOr<void> handle_DIVPD();
    InterruptRaisedOr<void> handle_DIVPS();
    InterruptRaisedOr<void> handle_DIVSD();
    InterruptRaisedOr<void> handle_DIVSS();
    InterruptRaisedOr<void> handle_DPPD();
    InterruptRaisedOr<void> handle_DPPS();
    InterruptRaisedOr<void> handle_EMMS();
    InterruptRaisedOr<void> handle_ENCODEKEY128();
    InterruptRaisedOr<void> handle_ENCODEKEY256();
    InterruptRaisedOr<void> handle_ENDBR32();
    InterruptRaisedOr<void> handle_ENDBR64();
    InterruptRaisedOr<void> handle_ENQCMD();
    InterruptRaisedOr<void> handle_ENQCMDS();
    InterruptRaisedOr<void> handle_ENTER();
    InterruptRaisedOr<void> handle_EXTRACTPS();
    InterruptRaisedOr<void> handle_F2XM1();
    InterruptRaisedOr<void> handle_FABS();
    InterruptRaisedOr<void> handle_FADD();
    InterruptRaisedOr<void> handle_FADDP();
    InterruptRaisedOr<void> handle_FBLD();
    InterruptRaisedOr<void> handle_FBSTP();
    InterruptRaisedOr<void> handle_FCHS();
    InterruptRaisedOr<void> handle_FCLEX();
    InterruptRaisedOr<void> handle_FCMOVcc();
    InterruptRaisedOr<void> handle_FCOM();
    InterruptRaisedOr<void> handle_FCOMI();
    InterruptRaisedOr<void> handle_FCOMIP();
    InterruptRaisedOr<void> handle_FCOMP();
    InterruptRaisedOr<void> handle_FCOMPP();
    InterruptRaisedOr<void> handle_FCOS();
    InterruptRaisedOr<void> handle_FDECSTP();
    InterruptRaisedOr<void> handle_FDIV();
    InterruptRaisedOr<void> handle_FDIVP();
    InterruptRaisedOr<void> handle_FDIVR();
    InterruptRaisedOr<void> handle_FDIVRP();
    InterruptRaisedOr<void> handle_FFREE();
    InterruptRaisedOr<void> handle_FIADD();
    InterruptRaisedOr<void> handle_FICOM();
    InterruptRaisedOr<void> handle_FICOMP();
    InterruptRaisedOr<void> handle_FIDIV();
    InterruptRaisedOr<void> handle_FIDIVR();
    InterruptRaisedOr<void> handle_FILD();
    InterruptRaisedOr<void> handle_FIMUL();
    InterruptRaisedOr<void> handle_FINCSTP();
    InterruptRaisedOr<void> handle_FINIT();
    InterruptRaisedOr<void> handle_FIST();
    InterruptRaisedOr<void> handle_FISTP();
    InterruptRaisedOr<void> handle_FISTTP();
    InterruptRaisedOr<void> handle_FISUB();
    InterruptRaisedOr<void> handle_FISUBR();
    InterruptRaisedOr<void> handle_FLD();
    InterruptRaisedOr<void> handle_FLD1();
    InterruptRaisedOr<void> handle_FLDCW();
    InterruptRaisedOr<void> handle_FLDENV();
    InterruptRaisedOr<void> handle_FLDL2E();
    InterruptRaisedOr<void> handle_FLDL2T();
    InterruptRaisedOr<void> handle_FLDLG2();
    InterruptRaisedOr<void> handle_FLDLN2();
    InterruptRaisedOr<void> handle_FLDPI();
    InterruptRaisedOr<void> handle_FLDZ();
    InterruptRaisedOr<void> handle_FMUL();
    InterruptRaisedOr<void> handle_FMULP();
    InterruptRaisedOr<void> handle_FNCLEX();
    InterruptRaisedOr<void> handle_FNINIT();
    InterruptRaisedOr<void> handle_FNOP();
    InterruptRaisedOr<void> handle_FNSAVE();
    InterruptRaisedOr<void> handle_FNSTCW();
    InterruptRaisedOr<void> handle_FNSTENV();
    InterruptRaisedOr<void> handle_FNSTSW();
    InterruptRaisedOr<void> handle_FPATAN();
    InterruptRaisedOr<void> handle_FPREM();
    InterruptRaisedOr<void> handle_FPREM1();
    InterruptRaisedOr<void> handle_FPTAN();
    InterruptRaisedOr<void> handle_FRNDINT();
    InterruptRaisedOr<void> handle_FRSTOR();
    InterruptRaisedOr<void> handle_FSAVE();
    InterruptRaisedOr<void> handle_FSCALE();
    InterruptRaisedOr<void> handle_FSIN();
    InterruptRaisedOr<void> handle_FSINCOS();
    InterruptRaisedOr<void> handle_FSQRT();
    InterruptRaisedOr<void> handle_FST();
    InterruptRaisedOr<void> handle_FSTCW();
    InterruptRaisedOr<void> handle_FSTENV();
    InterruptRaisedOr<void> handle_FSTP();
    InterruptRaisedOr<void> handle_FSTSW();
    InterruptRaisedOr<void> handle_FSUB();
    InterruptRaisedOr<void> handle_FSUBP();
    InterruptRaisedOr<void> handle_FSUBR();
    InterruptRaisedOr<void> handle_FSUBRP();
    InterruptRaisedOr<void> handle_FTST();
    InterruptRaisedOr<void> handle_FUCOM();
    InterruptRaisedOr<void> handle_FUCOMI();
    InterruptRaisedOr<void> handle_FUCOMIP();
    InterruptRaisedOr<void> handle_FUCOMP();
    InterruptRaisedOr<void> handle_FUCOMPP();
    InterruptRaisedOr<void> handle_FWAIT();
    InterruptRaisedOr<void> handle_FXAM();
    InterruptRaisedOr<void> handle_FXCH();
    InterruptRaisedOr<void> handle_FXRSTOR();
    InterruptRaisedOr<void> handle_FXSAVE();
    InterruptRaisedOr<void> handle_FXTRACT();
    InterruptRaisedOr<void> handle_FYL2X();
    InterruptRaisedOr<void> handle_FYL2XP1();
    InterruptRaisedOr<void> handle_GF2P8AFFINEINVQB();
    InterruptRaisedOr<void> handle_GF2P8AFFINEQB();
    InterruptRaisedOr<void> handle_GF2P8MULB();
    InterruptRaisedOr<void> handle_HADDPD();
    InterruptRaisedOr<void> handle_HADDPS();
    InterruptRaisedOr<void> handle_HLT();
    InterruptRaisedOr<void> handle_HRESET();
    InterruptRaisedOr<void> handle_HSUBPD();
    InterruptRaisedOr<void> handle_HSUBPS();
    InterruptRaisedOr<void> handle_IDIV();
    InterruptRaisedOr<void> handle_IMUL();
    InterruptRaisedOr<void> handle_IN();
    InterruptRaisedOr<void> handle_INC();
    InterruptRaisedOr<void> handle_INCSSPD();
    InterruptRaisedOr<void> handle_INCSSPQ();
    InterruptRaisedOr<void> handle_INS();
    InterruptRaisedOr<void> handle_INSB();
    InterruptRaisedOr<void> handle_INSD();
    InterruptRaisedOr<void> handle_INSERTPS();
    InterruptRaisedOr<void> handle_INSW();
    InterruptRaisedOr<void> handle_INT();
    InterruptRaisedOr<void> handle_INT1();
    InterruptRaisedOr<void> handle_INT3();
    InterruptRaisedOr<void> handle_INTO();
    InterruptRaisedOr<void> handle_INVD();
    InterruptRaisedOr<void> handle_INVLPG();
    InterruptRaisedOr<void> handle_INVPCID();
    InterruptRaisedOr<void> handle_IRET();
    InterruptRaisedOr<void> handle_IRETD();
    InterruptRaisedOr<void> handle_IRETQ();
    InterruptRaisedOr<void> handle_JMP();
    InterruptRaisedOr<void> handle_Jcc();
    InterruptRaisedOr<void> handle_KADDB();
    InterruptRaisedOr<void> handle_KADDD();
    InterruptRaisedOr<void> handle_KADDQ();
    InterruptRaisedOr<void> handle_KADDW();
    InterruptRaisedOr<void> handle_KANDB();
    InterruptRaisedOr<void> handle_KANDD();
    InterruptRaisedOr<void> handle_KANDNB();
    InterruptRaisedOr<void> handle_KANDND();
    InterruptRaisedOr<void> handle_KANDNQ();
    InterruptRaisedOr<void> handle_KANDNW();
    InterruptRaisedOr<void> handle_KANDQ();
    InterruptRaisedOr<void> handle_KANDW();
    InterruptRaisedOr<void> handle_KMOVB();
    InterruptRaisedOr<void> handle_KMOVD();
    InterruptRaisedOr<void> handle_KMOVQ();
    InterruptRaisedOr<void> handle_KMOVW();
    InterruptRaisedOr<void> handle_KNOTB();
    InterruptRaisedOr<void> handle_KNOTD();
    InterruptRaisedOr<void> handle_KNOTQ();
    InterruptRaisedOr<void> handle_KNOTW();
    InterruptRaisedOr<void> handle_KORB();
    InterruptRaisedOr<void> handle_KORD();
    InterruptRaisedOr<void> handle_KORQ();
    InterruptRaisedOr<void> handle_KORTESTB();
    InterruptRaisedOr<void> handle_KORTESTD();
    InterruptRaisedOr<void> handle_KORTESTQ();
    InterruptRaisedOr<void> handle_KORTESTW();
    InterruptRaisedOr<void> handle_KORW();
    InterruptRaisedOr<void> handle_KSHIFTLB();
    InterruptRaisedOr<void> handle_KSHIFTLD();
    InterruptRaisedOr<void> handle_KSHIFTLQ();
    InterruptRaisedOr<void> handle_KSHIFTLW();
    InterruptRaisedOr<void> handle_KSHIFTRB();
    InterruptRaisedOr<void> handle_KSHIFTRD();
    InterruptRaisedOr<void> handle_KSHIFTRQ();
    InterruptRaisedOr<void> handle_KSHIFTRW();
    InterruptRaisedOr<void> handle_KTESTB();
    InterruptRaisedOr<void> handle_KTESTD();
    InterruptRaisedOr<void> handle_KTESTQ();
    InterruptRaisedOr<void> handle_KTESTW();
    InterruptRaisedOr<void> handle_KUNPCKBW();
    InterruptRaisedOr<void> handle_KUNPCKDQ();
    InterruptRaisedOr<void> handle_KUNPCKWD();
    InterruptRaisedOr<void> handle_KXNORB();
    InterruptRaisedOr<void> handle_KXNORD();
    InterruptRaisedOr<void> handle_KXNORQ();
    InterruptRaisedOr<void> handle_KXNORW();
    InterruptRaisedOr<void> handle_KXORB();
    InterruptRaisedOr<void> handle_KXORD();
    InterruptRaisedOr<void> handle_KXORQ();
    InterruptRaisedOr<void> handle_KXORW();
    InterruptRaisedOr<void> handle_LAHF();
    InterruptRaisedOr<void> handle_LAR();
    InterruptRaisedOr<void> handle_LDDQU();
    InterruptRaisedOr<void> handle_LDMXCSR();
    InterruptRaisedOr<void> handle_LDS();
    InterruptRaisedOr<void> handle_LDTILECFG();
    InterruptRaisedOr<void> handle_LEA();
    InterruptRaisedOr<void> handle_LEAVE();
    InterruptRaisedOr<void> handle_LES();
    InterruptRaisedOr<void> handle_LFENCE();
    InterruptRaisedOr<void> handle_LFS();
    InterruptRaisedOr<void> handle_LGDT();
    InterruptRaisedOr<void> handle_LGS();
    InterruptRaisedOr<void> handle_LIDT();
    InterruptRaisedOr<void> handle_LLDT();
    InterruptRaisedOr<void> handle_LMSW();
    InterruptRaisedOr<void> handle_LOADIWKEY();
    InterruptRaisedOr<void> handle_LOCK();
    InterruptRaisedOr<void> handle_LODS();
    InterruptRaisedOr<void> handle_LODSB();
    InterruptRaisedOr<void> handle_LODSD();
    InterruptRaisedOr<void> handle_LODSQ();
    InterruptRaisedOr<void> handle_LODSW();
    InterruptRaisedOr<void> handle_LOOP();
    InterruptRaisedOr<void> handle_LOOPcc();
    InterruptRaisedOr<void> handle_LSL();
    InterruptRaisedOr<void> handle_LSS();
    InterruptRaisedOr<void> handle_LTR();
    InterruptRaisedOr<void> handle_LZCNT();
    InterruptRaisedOr<void> handle_MASKMOVDQU();
    InterruptRaisedOr<void> handle_MASKMOVQ();
    InterruptRaisedOr<void> handle_MAXPD();
    InterruptRaisedOr<void> handle_MAXPS();
    InterruptRaisedOr<void> handle_MAXSD();
    InterruptRaisedOr<void> handle_MAXSS();
    InterruptRaisedOr<void> handle_MFENCE();
    InterruptRaisedOr<void> handle_MINPD();
    InterruptRaisedOr<void> handle_MINPS();
    InterruptRaisedOr<void> handle_MINSD();
    InterruptRaisedOr<void> handle_MINSS();
    InterruptRaisedOr<void> handle_MONITOR();
    InterruptRaisedOr<void> handle_MOV();
    InterruptRaisedOr<void> handle_MOVAPD();
    InterruptRaisedOr<void> handle_MOVAPS();
    InterruptRaisedOr<void> handle_MOVBE();
    InterruptRaisedOr<void> handle_MOVD();
    InterruptRaisedOr<void> handle_MOVDDUP();
    InterruptRaisedOr<void> handle_MOVDIR64B();
    InterruptRaisedOr<void> handle_MOVDIRI();
    InterruptRaisedOr<void> handle_MOVDQ2Q();
    InterruptRaisedOr<void> handle_MOVDQA();
    InterruptRaisedOr<void> handle_MOVDQU();
    InterruptRaisedOr<void> handle_MOVHLPS();
    InterruptRaisedOr<void> handle_MOVHPD();
    InterruptRaisedOr<void> handle_MOVHPS();
    InterruptRaisedOr<void> handle_MOVLHPS();
    InterruptRaisedOr<void> handle_MOVLPD();
    InterruptRaisedOr<void> handle_MOVLPS();
    InterruptRaisedOr<void> handle_MOVMSKPD();
    InterruptRaisedOr<void> handle_MOVMSKPS();
    InterruptRaisedOr<void> handle_MOVNTDQ();
    InterruptRaisedOr<void> handle_MOVNTDQA();
    InterruptRaisedOr<void> handle_MOVNTI();
    InterruptRaisedOr<void> handle_MOVNTPD();
    InterruptRaisedOr<void> handle_MOVNTPS();
    InterruptRaisedOr<void> handle_MOVNTQ();
    InterruptRaisedOr<void> handle_MOVQ();
    InterruptRaisedOr<void> handle_MOVQ2DQ();
    InterruptRaisedOr<void> handle_MOVS();
    InterruptRaisedOr<void> handle_MOVSB();
    InterruptRaisedOr<void> handle_MOVSD();
    InterruptRaisedOr<void> handle_MOVSHDUP();
    InterruptRaisedOr<void> handle_MOVSLDUP();
    InterruptRaisedOr<void> handle_MOVSQ();
    InterruptRaisedOr<void> handle_MOVSS();
    InterruptRaisedOr<void> handle_MOVSW();
    InterruptRaisedOr<void> handle_MOVSX();
    InterruptRaisedOr<void> handle_MOVSXD();
    InterruptRaisedOr<void> handle_MOVUPD();
    InterruptRaisedOr<void> handle_MOVUPS();
    InterruptRaisedOr<void> handle_MOVZX();
    InterruptRaisedOr<void> handle_MPSADBW();
    InterruptRaisedOr<void> handle_MUL();
    InterruptRaisedOr<void> handle_MULPD();
    InterruptRaisedOr<void> handle_MULPS();
    InterruptRaisedOr<void> handle_MULSD();
    InterruptRaisedOr<void> handle_MULSS();
    InterruptRaisedOr<void> handle_MULX();
    InterruptRaisedOr<void> handle_MWAIT();
    InterruptRaisedOr<void> handle_NEG();
    InterruptRaisedOr<void> handle_NOP();
    InterruptRaisedOr<void> handle_NOT();
    InterruptRaisedOr<void> handle_OR();
    InterruptRaisedOr<void> handle_ORPD();
    InterruptRaisedOr<void> handle_ORPS();
    InterruptRaisedOr<void> handle_OUT();
    InterruptRaisedOr<void> handle_OUTS();
    InterruptRaisedOr<void> handle_OUTSB();
    InterruptRaisedOr<void> handle_OUTSD();
    InterruptRaisedOr<void> handle_OUTSW();
    InterruptRaisedOr<void> handle_PABSB();
    InterruptRaisedOr<void> handle_PABSD();
    InterruptRaisedOr<void> handle_PABSQ();
    InterruptRaisedOr<void> handle_PABSW();
    InterruptRaisedOr<void> handle_PACKSSDW();
    InterruptRaisedOr<void> handle_PACKSSWB();
    InterruptRaisedOr<void> handle_PACKUSDW();
    InterruptRaisedOr<void> handle_PACKUSWB();
    InterruptRaisedOr<void> handle_PADDB();
    InterruptRaisedOr<void> handle_PADDD();
    InterruptRaisedOr<void> handle_PADDQ();
    InterruptRaisedOr<void> handle_PADDSB();
    InterruptRaisedOr<void> handle_PADDSW();
    InterruptRaisedOr<void> handle_PADDUSB();
    InterruptRaisedOr<void> handle_PADDUSW();
    InterruptRaisedOr<void> handle_PADDW();
    InterruptRaisedOr<void> handle_PALIGNR();
    InterruptRaisedOr<void> handle_PAND();
    InterruptRaisedOr<void> handle_PANDN();
    InterruptRaisedOr<void> handle_PAUSE();
    InterruptRaisedOr<void> handle_PAVGB();
    InterruptRaisedOr<void> handle_PAVGW();
    InterruptRaisedOr<void> handle_PBLENDVB();
    InterruptRaisedOr<void> handle_PBLENDW();
    InterruptRaisedOr<void> handle_PCLMULQDQ();
    InterruptRaisedOr<void> handle_PCMPEQB();
    InterruptRaisedOr<void> handle_PCMPEQD();
    InterruptRaisedOr<void> handle_PCMPEQQ();
    InterruptRaisedOr<void> handle_PCMPEQW();
    InterruptRaisedOr<void> handle_PCMPESTRI();
    InterruptRaisedOr<void> handle_PCMPESTRM();
    InterruptRaisedOr<void> handle_PCMPGTB();
    InterruptRaisedOr<void> handle_PCMPGTD();
    InterruptRaisedOr<void> handle_PCMPGTQ();
    InterruptRaisedOr<void> handle_PCMPGTW();
    InterruptRaisedOr<void> handle_PCMPISTRI();
    InterruptRaisedOr<void> handle_PCMPISTRM();
    InterruptRaisedOr<void> handle_PCONFIG();
    InterruptRaisedOr<void> handle_PDEP();
    InterruptRaisedOr<void> handle_PEXT();
    InterruptRaisedOr<void> handle_PEXTRB();
    InterruptRaisedOr<void> handle_PEXTRD();
    InterruptRaisedOr<void> handle_PEXTRQ();
    InterruptRaisedOr<void> handle_PEXTRW();
    InterruptRaisedOr<void> handle_PHADDD();
    InterruptRaisedOr<void> handle_PHADDSW();
    InterruptRaisedOr<void> handle_PHADDW();
    InterruptRaisedOr<void> handle_PHMINPOSUW();
    InterruptRaisedOr<void> handle_PHSUBD();
    InterruptRaisedOr<void> handle_PHSUBSW();
    InterruptRaisedOr<void> handle_PHSUBW();
    InterruptRaisedOr<void> handle_PINSRB();
    InterruptRaisedOr<void> handle_PINSRD();
    InterruptRaisedOr<void> handle_PINSRQ();
    InterruptRaisedOr<void> handle_PINSRW();
    InterruptRaisedOr<void> handle_PMADDUBSW();
    InterruptRaisedOr<void> handle_PMADDWD();
    InterruptRaisedOr<void> handle_PMAXSB();
    InterruptRaisedOr<void> handle_PMAXSD();
    InterruptRaisedOr<void> handle_PMAXSQ();
    InterruptRaisedOr<void> handle_PMAXSW();
    InterruptRaisedOr<void> handle_PMAXUB();
    InterruptRaisedOr<void> handle_PMAXUD();
    InterruptRaisedOr<void> handle_PMAXUQ();
    InterruptRaisedOr<void> handle_PMAXUW();
    InterruptRaisedOr<void> handle_PMINSB();
    InterruptRaisedOr<void> handle_PMINSD();
    InterruptRaisedOr<void> handle_PMINSQ();
    InterruptRaisedOr<void> handle_PMINSW();
    InterruptRaisedOr<void> handle_PMINUB();
    InterruptRaisedOr<void> handle_PMINUD();
    InterruptRaisedOr<void> handle_PMINUQ();
    InterruptRaisedOr<void> handle_PMINUW();
    InterruptRaisedOr<void> handle_PMOVMSKB();
    InterruptRaisedOr<void> handle_PMOVSX();
    InterruptRaisedOr<void> handle_PMOVZX();
    InterruptRaisedOr<void> handle_PMULDQ();
    InterruptRaisedOr<void> handle_PMULHRSW();
    InterruptRaisedOr<void> handle_PMULHUW();
    InterruptRaisedOr<void> handle_PMULHW();
    InterruptRaisedOr<void> handle_PMULLD();
    InterruptRaisedOr<void> handle_PMULLQ();
    InterruptRaisedOr<void> handle_PMULLW();
    InterruptRaisedOr<void> handle_PMULUDQ();
    InterruptRaisedOr<void> handle_POP();
    InterruptRaisedOr<void> handle_POPA();
    InterruptRaisedOr<void> handle_POPAD();
    InterruptRaisedOr<void> handle_POPCNT();
    InterruptRaisedOr<void> handle_POPF();
    InterruptRaisedOr<void> handle_POPFD();
    InterruptRaisedOr<void> handle_POPFQ();
    InterruptRaisedOr<void> handle_POR();
    InterruptRaisedOr<void> handle_PREFETCHW();
    InterruptRaisedOr<void> handle_PREFETCHh();
    InterruptRaisedOr<void> handle_PSADBW();
    InterruptRaisedOr<void> handle_PSHUFB();
    InterruptRaisedOr<void> handle_PSHUFD();
    InterruptRaisedOr<void> handle_PSHUFHW();
    InterruptRaisedOr<void> handle_PSHUFLW();
    InterruptRaisedOr<void> handle_PSHUFW();
    InterruptRaisedOr<void> handle_PSIGNB();
    InterruptRaisedOr<void> handle_PSIGND();
    InterruptRaisedOr<void> handle_PSIGNW();
    InterruptRaisedOr<void> handle_PSLLD();
    InterruptRaisedOr<void> handle_PSLLDQ();
    InterruptRaisedOr<void> handle_PSLLQ();
    InterruptRaisedOr<void> handle_PSLLW();
    InterruptRaisedOr<void> handle_PSRAD();
    InterruptRaisedOr<void> handle_PSRAQ();
    InterruptRaisedOr<void> handle_PSRAW();
    InterruptRaisedOr<void> handle_PSRLD();
    InterruptRaisedOr<void> handle_PSRLDQ();
    InterruptRaisedOr<void> handle_PSRLQ();
    InterruptRaisedOr<void> handle_PSRLW();
    InterruptRaisedOr<void> handle_PSUBB();
    InterruptRaisedOr<void> handle_PSUBD();
    InterruptRaisedOr<void> handle_PSUBQ();
    InterruptRaisedOr<void> handle_PSUBSB();
    InterruptRaisedOr<void> handle_PSUBSW();
    InterruptRaisedOr<void> handle_PSUBUSB();
    InterruptRaisedOr<void> handle_PSUBUSW();
    InterruptRaisedOr<void> handle_PSUBW();
    InterruptRaisedOr<void> handle_PTEST();
    InterruptRaisedOr<void> handle_PTWRITE();
    InterruptRaisedOr<void> handle_PUNPCKHBW();
    InterruptRaisedOr<void> handle_PUNPCKHDQ();
    InterruptRaisedOr<void> handle_PUNPCKHQDQ();
    InterruptRaisedOr<void> handle_PUNPCKHWD();
    InterruptRaisedOr<void> handle_PUNPCKLBW();
    InterruptRaisedOr<void> handle_PUNPCKLDQ();
    InterruptRaisedOr<void> handle_PUNPCKLQDQ();
    InterruptRaisedOr<void> handle_PUNPCKLWD();
    InterruptRaisedOr<void> handle_PUSH();
    InterruptRaisedOr<void> handle_PUSHA();
    InterruptRaisedOr<void> handle_PUSHAD();
    InterruptRaisedOr<void> handle_PUSHF();
    InterruptRaisedOr<void> handle_PUSHFD();
    InterruptRaisedOr<void> handle_PUSHFQ();
    InterruptRaisedOr<void> handle_PXOR();
    InterruptRaisedOr<void> handle_RCL();
    InterruptRaisedOr<void> handle_RCPPS();
    InterruptRaisedOr<void> handle_RCPSS();
    InterruptRaisedOr<void> handle_RCR();
    InterruptRaisedOr<void> handle_RDFSBASE();
    InterruptRaisedOr<void> handle_RDGSBASE();
    InterruptRaisedOr<void> handle_RDMSR();
    InterruptRaisedOr<void> handle_RDPID();
    InterruptRaisedOr<void> handle_RDPKRU();
    InterruptRaisedOr<void> handle_RDPMC();
    InterruptRaisedOr<void> handle_RDRAND();
    InterruptRaisedOr<void> handle_RDSEED();
    InterruptRaisedOr<void> handle_RDSSPD();
    InterruptRaisedOr<void> handle_RDSSPQ();
    InterruptRaisedOr<void> handle_RDTSC();
    InterruptRaisedOr<void> handle_RDTSCP();
    InterruptRaisedOr<void> handle_REP();
    InterruptRaisedOr<void> handle_REPE();
    InterruptRaisedOr<void> handle_REPNE();
    InterruptRaisedOr<void> handle_REPNZ();
    InterruptRaisedOr<void> handle_REPZ();
    InterruptRaisedOr<void> handle_RET();
    InterruptRaisedOr<void> handle_ROL();
    InterruptRaisedOr<void> handle_ROR();
    InterruptRaisedOr<void> handle_RORX();
    InterruptRaisedOr<void> handle_ROUNDPD();
    InterruptRaisedOr<void> handle_ROUNDPS();
    InterruptRaisedOr<void> handle_ROUNDSD();
    InterruptRaisedOr<void> handle_ROUNDSS();
    InterruptRaisedOr<void> handle_RSM();
    InterruptRaisedOr<void> handle_RSQRTPS();
    InterruptRaisedOr<void> handle_RSQRTSS();
    InterruptRaisedOr<void> handle_RSTORSSP();
    InterruptRaisedOr<void> handle_SAHF();
    InterruptRaisedOr<void> handle_SAL();
    InterruptRaisedOr<void> handle_SAR();
    InterruptRaisedOr<void> handle_SARX();
    InterruptRaisedOr<void> handle_SAVEPREVSSP();
    InterruptRaisedOr<void> handle_SBB();
    InterruptRaisedOr<void> handle_SCAS();
    InterruptRaisedOr<void> handle_SCASB();
    InterruptRaisedOr<void> handle_SCASD();
    InterruptRaisedOr<void> handle_SCASW();
    InterruptRaisedOr<void> handle_SENDUIPI();
    InterruptRaisedOr<void> handle_SERIALIZE();
    InterruptRaisedOr<void> handle_SETSSBSY();
    InterruptRaisedOr<void> handle_SETcc();
    InterruptRaisedOr<void> handle_SFENCE();
    InterruptRaisedOr<void> handle_SGDT();
    InterruptRaisedOr<void> handle_SHA1MSG1();
    InterruptRaisedOr<void> handle_SHA1MSG2();
    InterruptRaisedOr<void> handle_SHA1NEXTE();
    InterruptRaisedOr<void> handle_SHA1RNDS4();
    InterruptRaisedOr<void> handle_SHA256MSG1();
    InterruptRaisedOr<void> handle_SHA256MSG2();
    InterruptRaisedOr<void> handle_SHA256RNDS2();
    InterruptRaisedOr<void> handle_SHL();
    InterruptRaisedOr<void> handle_SHLD();
    InterruptRaisedOr<void> handle_SHLX();
    InterruptRaisedOr<void> handle_SHR();
    InterruptRaisedOr<void> handle_SHRD();
    InterruptRaisedOr<void> handle_SHRX();
    InterruptRaisedOr<void> handle_SHUFPD();
    InterruptRaisedOr<void> handle_SHUFPS();
    InterruptRaisedOr<void> handle_SIDT();
    InterruptRaisedOr<void> handle_SLDT();
    InterruptRaisedOr<void> handle_SMSW();
    InterruptRaisedOr<void> handle_SQRTPD();
    InterruptRaisedOr<void> handle_SQRTPS();
    InterruptRaisedOr<void> handle_SQRTSD();
    InterruptRaisedOr<void> handle_SQRTSS();
    InterruptRaisedOr<void> handle_STAC();
    InterruptRaisedOr<void> handle_STC();
    InterruptRaisedOr<void> handle_STD();
    InterruptRaisedOr<void> handle_STI();
    InterruptRaisedOr<void> handle_STMXCSR();
    InterruptRaisedOr<void> handle_STOS();
    InterruptRaisedOr<void> handle_STOSB();
    InterruptRaisedOr<void> handle_STOSD();
    InterruptRaisedOr<void> handle_STOSQ();
    InterruptRaisedOr<void> handle_STOSW();
    InterruptRaisedOr<void> handle_STR();
    InterruptRaisedOr<void> handle_STTILECFG();
    InterruptRaisedOr<void> handle_STUI();
    InterruptRaisedOr<void> handle_SUB();
    InterruptRaisedOr<void> handle_SUBPD();
    InterruptRaisedOr<void> handle_SUBPS();
    InterruptRaisedOr<void> handle_SUBSD();
    InterruptRaisedOr<void> handle_SUBSS();
    InterruptRaisedOr<void> handle_SWAPGS();
    InterruptRaisedOr<void> handle_SYSCALL();
    InterruptRaisedOr<void> handle_SYSENTER();
    InterruptRaisedOr<void> handle_SYSEXIT();
    InterruptRaisedOr<void> handle_SYSRET();
    InterruptRaisedOr<void> handle_TDPBF16PS();
    InterruptRaisedOr<void> handle_TDPBSSD();
    InterruptRaisedOr<void> handle_TDPBSUD();
    InterruptRaisedOr<void> handle_TDPBUSD();
    InterruptRaisedOr<void> handle_TDPBUUD();
    InterruptRaisedOr<void> handle_TEST();
    InterruptRaisedOr<void> handle_TESTUI();
    InterruptRaisedOr<void> handle_TILELOADD();
    InterruptRaisedOr<void> handle_TILERELEASE();
    InterruptRaisedOr<void> handle_TILESTORED();
    InterruptRaisedOr<void> handle_TILEZERO();
    InterruptRaisedOr<void> handle_TPAUSE();
    InterruptRaisedOr<void> handle_TZCNT();
    InterruptRaisedOr<void> handle_UCOMISD();
    InterruptRaisedOr<void> handle_UCOMISS();
    InterruptRaisedOr<void> handle_UD();
    InterruptRaisedOr<void> handle_UIRET();

private:
    MMU m_mmu;

    PIC m_pic;
    // NOTE: we don't support the NMI pin

    ICU m_icu;
    Disassembler m_disassembler;

    std::optional<Interrupt> m_interrupt_to_be_handled;
    State m_state;

    // TODO: initialize all registers with sane default values
    //       See page 3425

    // General Purpose / Pointer Registers
    u64 m_rax = 0x0;
    u64 m_rbx = 0x0;
    u64 m_rcx = 0x0;
    u64 m_rdx = 0x00000600;
    u64 m_rsi = 0x0;
    u64 m_rsp = 0x0;
    u64 m_rbp = 0x0;
    u64 m_rip = 0x0000FFF0;
    u64 m_r8 = 0x0;
    u64 m_r9 = 0x0;
    u64 m_r10 = 0x0;
    u64 m_r11 = 0x0;
    u64 m_r12 = 0x0;
    u64 m_r13 = 0x0;
    u64 m_r14 = 0x0;
    u64 m_r15 = 0x0;


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


    /**
     * RFLAGS Registers
     */
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
    RFLAGS m_rflags;


    /**
     * Control Registers
     */
    struct CR0 {
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
    };
    static_assert(sizeof(CR0) == 8);
    CR0 m_cr0;

    // This control register contains the linear (virtual) address which triggered a page fault, available in the page fault's interrupt handler.
    VirtualAddress m_cr2;
    struct CR3 {
        u64 _pcid1 : 3;
        u64 PWT : 1;
        u64 PCD : 1;
        u64 _pcid2 : 7;
        u64 pml4_base_paddr : MAXPHYADDR - 12; // Physical Base Address of the PML4
        u64 __reserved1 : 60 - MAXPHYADDR = 0; // Reserved (must be 0)
        u64 LAM_U57 : 1; // When set, enables LAM57 (masking of linear-address bits 62:57) for user pointers and overrides CR3.LAM_U48.
        u64 LAM_U48 : 1; // When set and CR3.LAM_U57 is clear, enables LAM48 (masking of linear-address bits 62:48) for user pointers.
        u64 __reserved2 : 1 = 0; // Reserved (must be 0)

        u64 reserved_bits_ored() const { return __reserved1 || __reserved2; }

        // // We have to do it this way cause of bit-field limitations/alignment issues
        PCID pcid() const { return ((u16)_pcid2 << 5) || PCD << 4 || PWT << 3 || _pcid1; }
        void set_pcid(PCID pcid) {
            _pcid1 = bits(pcid, 2, 0);
            PWT = bits(pcid, 3, 3);
            PCD = bits(pcid, 4, 4);
            _pcid2 = bits(pcid, 11, 5);
        }
    };
    static_assert(sizeof(CR3) == 8);
    CR3 m_cr3;

    struct CR4 {
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

        u64 reserved_bits_ored() const { return __reserved1 || __reserved2; }
    };
    static_assert(sizeof(CR4) == 8);
    CR4 m_cr4;

    struct CR8 {
        u64 TPR : 4; // This sets the threshold value corresponding to the highestpriority interrupt to be blocked. A value of 0 means all interrupts are enabled. This field is available in 64-bit mode. A value of 15 means all interrupts will be disabled.
    };
    static_assert(sizeof(CR8) == 8);
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
        u64 : 7; // Reserved
        u64 LME : 2; // Long Mode Enable
        u64 LMA : 1; // Long Mode Active
        u64 NXE : 1; // No-Execute Enable
        u64 SVME : 1; // Secure Virtual Machine Enable
        u64 LMSLE : 1; // Long Mode Segment Limit Enable
        u64 FFXSR : 1; // Fast FXSAVE/FXRSTOR
        u64 TCE : 1; // Translation Cache Extension u8 16 - 63 0 Reserved
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
};


}