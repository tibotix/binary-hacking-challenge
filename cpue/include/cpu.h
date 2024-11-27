#pragma once

#include <stack>

#include "mmu.h"
#include "icu.h"
#include "address.h"
#include "disassembler.h"
#include "pic.h"
#include "segmentation.h"
#include "forward.h"


namespace CPUE {

// TODO: when implementing protected instructions, see chapter 2.8: System Instruction Summary
// TODO: when implementing some instructions, look at type checking (chapter 6.4 or page 3250)


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
    [[nodiscard]] InterruptRaisedOr<void> stack_push(u64 value);
    [[nodiscard]] InterruptRaisedOr<u64> stack_pop();


    [[nodiscard]] InterruptRaisedOr<void> load_segment_register(SegmentRegisterAlias alias, SegmentSelector selector);
    [[nodiscard]] InterruptRaisedOr<void> load_segment_register(SegmentRegisterAlias alias, SegmentSelector selector, GDTLDTDescriptor const& descriptor);

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
    [[nodiscard]] InterruptRaisedOr<void> handle_nested_interrupt(Interrupt i);
    [[nodiscard]] InterruptRaisedOr<void> handle_interrupt(Interrupt i);
    // NOTE: as InterruptGateDescriptor and TrapGateDescriptor have the same layout, we simply choose one to receive
    [[nodiscard]] InterruptRaisedOr<void> enter_interrupt_trap_gate(Interrupt const& i, TrapGateDescriptor const& descriptor);
    [[nodiscard]] InterruptRaisedOr<void> enter_task_gate(Interrupt const& i, TaskGateDescriptor const& task_gate_descriptor);
    [[nodiscard]] InterruptRaisedOr<void> enter_call_gate(SegmentSelector const& selector, CallGateDescriptor const& call_gate_descriptor, bool through_call_insn);
    [[nodiscard]] InterruptRaisedOr<std::pair<SegmentSelector, u64>> do_stack_switch(u8 target_pl);

    [[nodiscard]] bool alignment_check_enabled() const { return m_cr0.AM && m_rflags.AC && cpl() == 3; }
    [[nodiscard]] InterruptRaisedOr<void> do_canonicality_check(VirtualAddress const& vaddr);

    DescriptorTable descriptor_table_of_selector(SegmentSelector selector) const;

private:
    [[nodiscard]] InterruptRaisedOr<void> handle_AAA();
    [[nodiscard]] InterruptRaisedOr<void> handle_AAD();
    [[nodiscard]] InterruptRaisedOr<void> handle_AAM();
    [[nodiscard]] InterruptRaisedOr<void> handle_AAS();
    [[nodiscard]] InterruptRaisedOr<void> handle_ADC();
    [[nodiscard]] InterruptRaisedOr<void> handle_ADCX();
    [[nodiscard]] InterruptRaisedOr<void> handle_ADD();
    [[nodiscard]] InterruptRaisedOr<void> handle_ADDPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_ADDPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_ADDSD();
    [[nodiscard]] InterruptRaisedOr<void> handle_ADDSS();
    [[nodiscard]] InterruptRaisedOr<void> handle_ADDSUBPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_ADDSUBPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_ADOX();
    [[nodiscard]] InterruptRaisedOr<void> handle_AESDEC();
    [[nodiscard]] InterruptRaisedOr<void> handle_AESDEC128KL();
    [[nodiscard]] InterruptRaisedOr<void> handle_AESDEC256KL();
    [[nodiscard]] InterruptRaisedOr<void> handle_AESDECLAST();
    [[nodiscard]] InterruptRaisedOr<void> handle_AESDECWIDE128KL();
    [[nodiscard]] InterruptRaisedOr<void> handle_AESDECWIDE256KL();
    [[nodiscard]] InterruptRaisedOr<void> handle_AESENC();
    [[nodiscard]] InterruptRaisedOr<void> handle_AESENC128KL();
    [[nodiscard]] InterruptRaisedOr<void> handle_AESENC256KL();
    [[nodiscard]] InterruptRaisedOr<void> handle_AESENCLAST();
    [[nodiscard]] InterruptRaisedOr<void> handle_AESENCWIDE128KL();
    [[nodiscard]] InterruptRaisedOr<void> handle_AESENCWIDE256KL();
    [[nodiscard]] InterruptRaisedOr<void> handle_AESIMC();
    [[nodiscard]] InterruptRaisedOr<void> handle_AESKEYGENASSIST();
    [[nodiscard]] InterruptRaisedOr<void> handle_AND();
    [[nodiscard]] InterruptRaisedOr<void> handle_ANDN();
    [[nodiscard]] InterruptRaisedOr<void> handle_ANDNPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_ANDNPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_ANDPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_ANDPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_ARPL();
    [[nodiscard]] InterruptRaisedOr<void> handle_BEXTR();
    [[nodiscard]] InterruptRaisedOr<void> handle_BLENDPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_BLENDPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_BLENDVPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_BLENDVPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_BLSI();
    [[nodiscard]] InterruptRaisedOr<void> handle_BLSMSK();
    [[nodiscard]] InterruptRaisedOr<void> handle_BLSR();
    [[nodiscard]] InterruptRaisedOr<void> handle_BNDCL();
    [[nodiscard]] InterruptRaisedOr<void> handle_BNDCN();
    [[nodiscard]] InterruptRaisedOr<void> handle_BNDCU();
    [[nodiscard]] InterruptRaisedOr<void> handle_BNDLDX();
    [[nodiscard]] InterruptRaisedOr<void> handle_BNDMK();
    [[nodiscard]] InterruptRaisedOr<void> handle_BNDMOV();
    [[nodiscard]] InterruptRaisedOr<void> handle_BNDSTX();
    [[nodiscard]] InterruptRaisedOr<void> handle_BOUND();
    [[nodiscard]] InterruptRaisedOr<void> handle_BSF();
    [[nodiscard]] InterruptRaisedOr<void> handle_BSR();
    [[nodiscard]] InterruptRaisedOr<void> handle_BSWAP();
    [[nodiscard]] InterruptRaisedOr<void> handle_BT();
    [[nodiscard]] InterruptRaisedOr<void> handle_BTC();
    [[nodiscard]] InterruptRaisedOr<void> handle_BTR();
    [[nodiscard]] InterruptRaisedOr<void> handle_BTS();
    [[nodiscard]] InterruptRaisedOr<void> handle_BZHI();
    [[nodiscard]] InterruptRaisedOr<void> handle_CALL();
    [[nodiscard]] InterruptRaisedOr<void> handle_CBW();
    [[nodiscard]] InterruptRaisedOr<void> handle_CDQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_CDQE();
    [[nodiscard]] InterruptRaisedOr<void> handle_CLAC();
    [[nodiscard]] InterruptRaisedOr<void> handle_CLC();
    [[nodiscard]] InterruptRaisedOr<void> handle_CLD();
    [[nodiscard]] InterruptRaisedOr<void> handle_CLDEMOTE();
    [[nodiscard]] InterruptRaisedOr<void> handle_CLFLUSH();
    [[nodiscard]] InterruptRaisedOr<void> handle_CLFLUSHOPT();
    [[nodiscard]] InterruptRaisedOr<void> handle_CLI();
    [[nodiscard]] InterruptRaisedOr<void> handle_CLRSSBSY();
    [[nodiscard]] InterruptRaisedOr<void> handle_CLTS();
    [[nodiscard]] InterruptRaisedOr<void> handle_CLUI();
    [[nodiscard]] InterruptRaisedOr<void> handle_CLWB();
    [[nodiscard]] InterruptRaisedOr<void> handle_CMC();
    [[nodiscard]] InterruptRaisedOr<void> handle_CMOVcc();
    [[nodiscard]] InterruptRaisedOr<void> handle_CMP();
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPSB();
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPSD();
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPSQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPSS();
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPXCHG();
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPXCHG16B();
    [[nodiscard]] InterruptRaisedOr<void> handle_CMPXCHG8B();
    [[nodiscard]] InterruptRaisedOr<void> handle_COMISD();
    [[nodiscard]] InterruptRaisedOr<void> handle_COMISS();
    [[nodiscard]] InterruptRaisedOr<void> handle_CPUID();
    [[nodiscard]] InterruptRaisedOr<void> handle_CQO();
    [[nodiscard]] InterruptRaisedOr<void> handle_CRC32();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTDQ2PD();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTDQ2PS();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTPD2DQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTPD2PI();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTPD2PS();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTPI2PD();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTPI2PS();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTPS2DQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTPS2PD();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTPS2PI();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTSD2SI();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTSD2SS();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTSI2SD();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTSI2SS();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTSS2SD();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTSS2SI();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTTPD2DQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTTPD2PI();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTTPS2DQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTTPS2PI();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTTSD2SI();
    [[nodiscard]] InterruptRaisedOr<void> handle_CVTTSS2SI();
    [[nodiscard]] InterruptRaisedOr<void> handle_CWD();
    [[nodiscard]] InterruptRaisedOr<void> handle_CWDE();
    [[nodiscard]] InterruptRaisedOr<void> handle_DAA();
    [[nodiscard]] InterruptRaisedOr<void> handle_DAS();
    [[nodiscard]] InterruptRaisedOr<void> handle_DEC();
    [[nodiscard]] InterruptRaisedOr<void> handle_DIV();
    [[nodiscard]] InterruptRaisedOr<void> handle_DIVPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_DIVPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_DIVSD();
    [[nodiscard]] InterruptRaisedOr<void> handle_DIVSS();
    [[nodiscard]] InterruptRaisedOr<void> handle_DPPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_DPPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_EMMS();
    [[nodiscard]] InterruptRaisedOr<void> handle_ENCODEKEY128();
    [[nodiscard]] InterruptRaisedOr<void> handle_ENCODEKEY256();
    [[nodiscard]] InterruptRaisedOr<void> handle_ENDBR32();
    [[nodiscard]] InterruptRaisedOr<void> handle_ENDBR64();
    [[nodiscard]] InterruptRaisedOr<void> handle_ENQCMD();
    [[nodiscard]] InterruptRaisedOr<void> handle_ENQCMDS();
    [[nodiscard]] InterruptRaisedOr<void> handle_ENTER();
    [[nodiscard]] InterruptRaisedOr<void> handle_EXTRACTPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_F2XM1();
    [[nodiscard]] InterruptRaisedOr<void> handle_FABS();
    [[nodiscard]] InterruptRaisedOr<void> handle_FADD();
    [[nodiscard]] InterruptRaisedOr<void> handle_FADDP();
    [[nodiscard]] InterruptRaisedOr<void> handle_FBLD();
    [[nodiscard]] InterruptRaisedOr<void> handle_FBSTP();
    [[nodiscard]] InterruptRaisedOr<void> handle_FCHS();
    [[nodiscard]] InterruptRaisedOr<void> handle_FCLEX();
    [[nodiscard]] InterruptRaisedOr<void> handle_FCMOVcc();
    [[nodiscard]] InterruptRaisedOr<void> handle_FCOM();
    [[nodiscard]] InterruptRaisedOr<void> handle_FCOMI();
    [[nodiscard]] InterruptRaisedOr<void> handle_FCOMIP();
    [[nodiscard]] InterruptRaisedOr<void> handle_FCOMP();
    [[nodiscard]] InterruptRaisedOr<void> handle_FCOMPP();
    [[nodiscard]] InterruptRaisedOr<void> handle_FCOS();
    [[nodiscard]] InterruptRaisedOr<void> handle_FDECSTP();
    [[nodiscard]] InterruptRaisedOr<void> handle_FDIV();
    [[nodiscard]] InterruptRaisedOr<void> handle_FDIVP();
    [[nodiscard]] InterruptRaisedOr<void> handle_FDIVR();
    [[nodiscard]] InterruptRaisedOr<void> handle_FDIVRP();
    [[nodiscard]] InterruptRaisedOr<void> handle_FFREE();
    [[nodiscard]] InterruptRaisedOr<void> handle_FIADD();
    [[nodiscard]] InterruptRaisedOr<void> handle_FICOM();
    [[nodiscard]] InterruptRaisedOr<void> handle_FICOMP();
    [[nodiscard]] InterruptRaisedOr<void> handle_FIDIV();
    [[nodiscard]] InterruptRaisedOr<void> handle_FIDIVR();
    [[nodiscard]] InterruptRaisedOr<void> handle_FILD();
    [[nodiscard]] InterruptRaisedOr<void> handle_FIMUL();
    [[nodiscard]] InterruptRaisedOr<void> handle_FINCSTP();
    [[nodiscard]] InterruptRaisedOr<void> handle_FINIT();
    [[nodiscard]] InterruptRaisedOr<void> handle_FIST();
    [[nodiscard]] InterruptRaisedOr<void> handle_FISTP();
    [[nodiscard]] InterruptRaisedOr<void> handle_FISTTP();
    [[nodiscard]] InterruptRaisedOr<void> handle_FISUB();
    [[nodiscard]] InterruptRaisedOr<void> handle_FISUBR();
    [[nodiscard]] InterruptRaisedOr<void> handle_FLD();
    [[nodiscard]] InterruptRaisedOr<void> handle_FLD1();
    [[nodiscard]] InterruptRaisedOr<void> handle_FLDCW();
    [[nodiscard]] InterruptRaisedOr<void> handle_FLDENV();
    [[nodiscard]] InterruptRaisedOr<void> handle_FLDL2E();
    [[nodiscard]] InterruptRaisedOr<void> handle_FLDL2T();
    [[nodiscard]] InterruptRaisedOr<void> handle_FLDLG2();
    [[nodiscard]] InterruptRaisedOr<void> handle_FLDLN2();
    [[nodiscard]] InterruptRaisedOr<void> handle_FLDPI();
    [[nodiscard]] InterruptRaisedOr<void> handle_FLDZ();
    [[nodiscard]] InterruptRaisedOr<void> handle_FMUL();
    [[nodiscard]] InterruptRaisedOr<void> handle_FMULP();
    [[nodiscard]] InterruptRaisedOr<void> handle_FNCLEX();
    [[nodiscard]] InterruptRaisedOr<void> handle_FNINIT();
    [[nodiscard]] InterruptRaisedOr<void> handle_FNOP();
    [[nodiscard]] InterruptRaisedOr<void> handle_FNSAVE();
    [[nodiscard]] InterruptRaisedOr<void> handle_FNSTCW();
    [[nodiscard]] InterruptRaisedOr<void> handle_FNSTENV();
    [[nodiscard]] InterruptRaisedOr<void> handle_FNSTSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_FPATAN();
    [[nodiscard]] InterruptRaisedOr<void> handle_FPREM();
    [[nodiscard]] InterruptRaisedOr<void> handle_FPREM1();
    [[nodiscard]] InterruptRaisedOr<void> handle_FPTAN();
    [[nodiscard]] InterruptRaisedOr<void> handle_FRNDINT();
    [[nodiscard]] InterruptRaisedOr<void> handle_FRSTOR();
    [[nodiscard]] InterruptRaisedOr<void> handle_FSAVE();
    [[nodiscard]] InterruptRaisedOr<void> handle_FSCALE();
    [[nodiscard]] InterruptRaisedOr<void> handle_FSIN();
    [[nodiscard]] InterruptRaisedOr<void> handle_FSINCOS();
    [[nodiscard]] InterruptRaisedOr<void> handle_FSQRT();
    [[nodiscard]] InterruptRaisedOr<void> handle_FST();
    [[nodiscard]] InterruptRaisedOr<void> handle_FSTCW();
    [[nodiscard]] InterruptRaisedOr<void> handle_FSTENV();
    [[nodiscard]] InterruptRaisedOr<void> handle_FSTP();
    [[nodiscard]] InterruptRaisedOr<void> handle_FSTSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_FSUB();
    [[nodiscard]] InterruptRaisedOr<void> handle_FSUBP();
    [[nodiscard]] InterruptRaisedOr<void> handle_FSUBR();
    [[nodiscard]] InterruptRaisedOr<void> handle_FSUBRP();
    [[nodiscard]] InterruptRaisedOr<void> handle_FTST();
    [[nodiscard]] InterruptRaisedOr<void> handle_FUCOM();
    [[nodiscard]] InterruptRaisedOr<void> handle_FUCOMI();
    [[nodiscard]] InterruptRaisedOr<void> handle_FUCOMIP();
    [[nodiscard]] InterruptRaisedOr<void> handle_FUCOMP();
    [[nodiscard]] InterruptRaisedOr<void> handle_FUCOMPP();
    [[nodiscard]] InterruptRaisedOr<void> handle_FWAIT();
    [[nodiscard]] InterruptRaisedOr<void> handle_FXAM();
    [[nodiscard]] InterruptRaisedOr<void> handle_FXCH();
    [[nodiscard]] InterruptRaisedOr<void> handle_FXRSTOR();
    [[nodiscard]] InterruptRaisedOr<void> handle_FXSAVE();
    [[nodiscard]] InterruptRaisedOr<void> handle_FXTRACT();
    [[nodiscard]] InterruptRaisedOr<void> handle_FYL2X();
    [[nodiscard]] InterruptRaisedOr<void> handle_FYL2XP1();
    [[nodiscard]] InterruptRaisedOr<void> handle_GF2P8AFFINEINVQB();
    [[nodiscard]] InterruptRaisedOr<void> handle_GF2P8AFFINEQB();
    [[nodiscard]] InterruptRaisedOr<void> handle_GF2P8MULB();
    [[nodiscard]] InterruptRaisedOr<void> handle_HADDPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_HADDPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_HLT();
    [[nodiscard]] InterruptRaisedOr<void> handle_HRESET();
    [[nodiscard]] InterruptRaisedOr<void> handle_HSUBPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_HSUBPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_IDIV();
    [[nodiscard]] InterruptRaisedOr<void> handle_IMUL();
    [[nodiscard]] InterruptRaisedOr<void> handle_IN();
    [[nodiscard]] InterruptRaisedOr<void> handle_INC();
    [[nodiscard]] InterruptRaisedOr<void> handle_INCSSPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_INCSSPQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_INS();
    [[nodiscard]] InterruptRaisedOr<void> handle_INSB();
    [[nodiscard]] InterruptRaisedOr<void> handle_INSD();
    [[nodiscard]] InterruptRaisedOr<void> handle_INSERTPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_INSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_INT();
    [[nodiscard]] InterruptRaisedOr<void> handle_INT1();
    [[nodiscard]] InterruptRaisedOr<void> handle_INT3();
    [[nodiscard]] InterruptRaisedOr<void> handle_INTO();
    [[nodiscard]] InterruptRaisedOr<void> handle_INVD();
    [[nodiscard]] InterruptRaisedOr<void> handle_INVLPG();
    [[nodiscard]] InterruptRaisedOr<void> handle_INVPCID();
    [[nodiscard]] InterruptRaisedOr<void> handle_IRET();
    [[nodiscard]] InterruptRaisedOr<void> handle_IRETD();
    [[nodiscard]] InterruptRaisedOr<void> handle_IRETQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_JMP();
    [[nodiscard]] InterruptRaisedOr<void> handle_Jcc();
    [[nodiscard]] InterruptRaisedOr<void> handle_KADDB();
    [[nodiscard]] InterruptRaisedOr<void> handle_KADDD();
    [[nodiscard]] InterruptRaisedOr<void> handle_KADDQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_KADDW();
    [[nodiscard]] InterruptRaisedOr<void> handle_KANDB();
    [[nodiscard]] InterruptRaisedOr<void> handle_KANDD();
    [[nodiscard]] InterruptRaisedOr<void> handle_KANDNB();
    [[nodiscard]] InterruptRaisedOr<void> handle_KANDND();
    [[nodiscard]] InterruptRaisedOr<void> handle_KANDNQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_KANDNW();
    [[nodiscard]] InterruptRaisedOr<void> handle_KANDQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_KANDW();
    [[nodiscard]] InterruptRaisedOr<void> handle_KMOVB();
    [[nodiscard]] InterruptRaisedOr<void> handle_KMOVD();
    [[nodiscard]] InterruptRaisedOr<void> handle_KMOVQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_KMOVW();
    [[nodiscard]] InterruptRaisedOr<void> handle_KNOTB();
    [[nodiscard]] InterruptRaisedOr<void> handle_KNOTD();
    [[nodiscard]] InterruptRaisedOr<void> handle_KNOTQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_KNOTW();
    [[nodiscard]] InterruptRaisedOr<void> handle_KORB();
    [[nodiscard]] InterruptRaisedOr<void> handle_KORD();
    [[nodiscard]] InterruptRaisedOr<void> handle_KORQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_KORTESTB();
    [[nodiscard]] InterruptRaisedOr<void> handle_KORTESTD();
    [[nodiscard]] InterruptRaisedOr<void> handle_KORTESTQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_KORTESTW();
    [[nodiscard]] InterruptRaisedOr<void> handle_KORW();
    [[nodiscard]] InterruptRaisedOr<void> handle_KSHIFTLB();
    [[nodiscard]] InterruptRaisedOr<void> handle_KSHIFTLD();
    [[nodiscard]] InterruptRaisedOr<void> handle_KSHIFTLQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_KSHIFTLW();
    [[nodiscard]] InterruptRaisedOr<void> handle_KSHIFTRB();
    [[nodiscard]] InterruptRaisedOr<void> handle_KSHIFTRD();
    [[nodiscard]] InterruptRaisedOr<void> handle_KSHIFTRQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_KSHIFTRW();
    [[nodiscard]] InterruptRaisedOr<void> handle_KTESTB();
    [[nodiscard]] InterruptRaisedOr<void> handle_KTESTD();
    [[nodiscard]] InterruptRaisedOr<void> handle_KTESTQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_KTESTW();
    [[nodiscard]] InterruptRaisedOr<void> handle_KUNPCKBW();
    [[nodiscard]] InterruptRaisedOr<void> handle_KUNPCKDQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_KUNPCKWD();
    [[nodiscard]] InterruptRaisedOr<void> handle_KXNORB();
    [[nodiscard]] InterruptRaisedOr<void> handle_KXNORD();
    [[nodiscard]] InterruptRaisedOr<void> handle_KXNORQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_KXNORW();
    [[nodiscard]] InterruptRaisedOr<void> handle_KXORB();
    [[nodiscard]] InterruptRaisedOr<void> handle_KXORD();
    [[nodiscard]] InterruptRaisedOr<void> handle_KXORQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_KXORW();
    [[nodiscard]] InterruptRaisedOr<void> handle_LAHF();
    [[nodiscard]] InterruptRaisedOr<void> handle_LAR();
    [[nodiscard]] InterruptRaisedOr<void> handle_LDDQU();
    [[nodiscard]] InterruptRaisedOr<void> handle_LDMXCSR();
    [[nodiscard]] InterruptRaisedOr<void> handle_LDS();
    [[nodiscard]] InterruptRaisedOr<void> handle_LDTILECFG();
    [[nodiscard]] InterruptRaisedOr<void> handle_LEA();
    [[nodiscard]] InterruptRaisedOr<void> handle_LEAVE();
    [[nodiscard]] InterruptRaisedOr<void> handle_LES();
    [[nodiscard]] InterruptRaisedOr<void> handle_LFENCE();
    [[nodiscard]] InterruptRaisedOr<void> handle_LFS();
    [[nodiscard]] InterruptRaisedOr<void> handle_LGDT();
    [[nodiscard]] InterruptRaisedOr<void> handle_LGS();
    [[nodiscard]] InterruptRaisedOr<void> handle_LIDT();
    [[nodiscard]] InterruptRaisedOr<void> handle_LLDT();
    [[nodiscard]] InterruptRaisedOr<void> handle_LMSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_LOADIWKEY();
    [[nodiscard]] InterruptRaisedOr<void> handle_LOCK();
    [[nodiscard]] InterruptRaisedOr<void> handle_LODS();
    [[nodiscard]] InterruptRaisedOr<void> handle_LODSB();
    [[nodiscard]] InterruptRaisedOr<void> handle_LODSD();
    [[nodiscard]] InterruptRaisedOr<void> handle_LODSQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_LODSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_LOOP();
    [[nodiscard]] InterruptRaisedOr<void> handle_LOOPcc();
    [[nodiscard]] InterruptRaisedOr<void> handle_LSL();
    [[nodiscard]] InterruptRaisedOr<void> handle_LSS();
    [[nodiscard]] InterruptRaisedOr<void> handle_LTR();
    [[nodiscard]] InterruptRaisedOr<void> handle_LZCNT();
    [[nodiscard]] InterruptRaisedOr<void> handle_MASKMOVDQU();
    [[nodiscard]] InterruptRaisedOr<void> handle_MASKMOVQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_MAXPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_MAXPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_MAXSD();
    [[nodiscard]] InterruptRaisedOr<void> handle_MAXSS();
    [[nodiscard]] InterruptRaisedOr<void> handle_MFENCE();
    [[nodiscard]] InterruptRaisedOr<void> handle_MINPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_MINPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_MINSD();
    [[nodiscard]] InterruptRaisedOr<void> handle_MINSS();
    [[nodiscard]] InterruptRaisedOr<void> handle_MONITOR();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOV();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVAPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVAPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVBE();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVD();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVDDUP();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVDIR64B();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVDIRI();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVDQ2Q();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVDQA();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVDQU();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVHLPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVHPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVHPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVLHPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVLPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVLPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVMSKPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVMSKPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVNTDQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVNTDQA();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVNTI();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVNTPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVNTPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVNTQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVQ2DQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVS();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVSB();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVSD();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVSHDUP();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVSLDUP();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVSQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVSS();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVSX();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVSXD();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVUPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVUPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_MOVZX();
    [[nodiscard]] InterruptRaisedOr<void> handle_MPSADBW();
    [[nodiscard]] InterruptRaisedOr<void> handle_MUL();
    [[nodiscard]] InterruptRaisedOr<void> handle_MULPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_MULPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_MULSD();
    [[nodiscard]] InterruptRaisedOr<void> handle_MULSS();
    [[nodiscard]] InterruptRaisedOr<void> handle_MULX();
    [[nodiscard]] InterruptRaisedOr<void> handle_MWAIT();
    [[nodiscard]] InterruptRaisedOr<void> handle_NEG();
    [[nodiscard]] InterruptRaisedOr<void> handle_NOP();
    [[nodiscard]] InterruptRaisedOr<void> handle_NOT();
    [[nodiscard]] InterruptRaisedOr<void> handle_OR();
    [[nodiscard]] InterruptRaisedOr<void> handle_ORPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_ORPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_OUT();
    [[nodiscard]] InterruptRaisedOr<void> handle_OUTS();
    [[nodiscard]] InterruptRaisedOr<void> handle_OUTSB();
    [[nodiscard]] InterruptRaisedOr<void> handle_OUTSD();
    [[nodiscard]] InterruptRaisedOr<void> handle_OUTSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PABSB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PABSD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PABSQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PABSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PACKSSDW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PACKSSWB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PACKUSDW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PACKUSWB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PADDB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PADDD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PADDQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PADDSB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PADDSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PADDUSB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PADDUSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PADDW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PALIGNR();
    [[nodiscard]] InterruptRaisedOr<void> handle_PAND();
    [[nodiscard]] InterruptRaisedOr<void> handle_PANDN();
    [[nodiscard]] InterruptRaisedOr<void> handle_PAUSE();
    [[nodiscard]] InterruptRaisedOr<void> handle_PAVGB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PAVGW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PBLENDVB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PBLENDW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PCLMULQDQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPEQB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPEQD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPEQQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPEQW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPESTRI();
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPESTRM();
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPGTB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPGTD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPGTQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPGTW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPISTRI();
    [[nodiscard]] InterruptRaisedOr<void> handle_PCMPISTRM();
    [[nodiscard]] InterruptRaisedOr<void> handle_PCONFIG();
    [[nodiscard]] InterruptRaisedOr<void> handle_PDEP();
    [[nodiscard]] InterruptRaisedOr<void> handle_PEXT();
    [[nodiscard]] InterruptRaisedOr<void> handle_PEXTRB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PEXTRD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PEXTRQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PEXTRW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PHADDD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PHADDSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PHADDW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PHMINPOSUW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PHSUBD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PHSUBSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PHSUBW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PINSRB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PINSRD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PINSRQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PINSRW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMADDUBSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMADDWD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMAXSB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMAXSD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMAXSQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMAXSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMAXUB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMAXUD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMAXUQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMAXUW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMINSB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMINSD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMINSQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMINSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMINUB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMINUD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMINUQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMINUW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMOVMSKB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMOVSX();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMOVZX();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMULDQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMULHRSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMULHUW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMULHW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMULLD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMULLQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMULLW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PMULUDQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_POP();
    [[nodiscard]] InterruptRaisedOr<void> handle_POPA();
    [[nodiscard]] InterruptRaisedOr<void> handle_POPAD();
    [[nodiscard]] InterruptRaisedOr<void> handle_POPCNT();
    [[nodiscard]] InterruptRaisedOr<void> handle_POPF();
    [[nodiscard]] InterruptRaisedOr<void> handle_POPFD();
    [[nodiscard]] InterruptRaisedOr<void> handle_POPFQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_POR();
    [[nodiscard]] InterruptRaisedOr<void> handle_PREFETCHW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PREFETCHh();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSADBW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSHUFB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSHUFD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSHUFHW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSHUFLW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSHUFW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSIGNB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSIGND();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSIGNW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSLLD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSLLDQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSLLQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSLLW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSRAD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSRAQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSRAW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSRLD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSRLDQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSRLQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSRLW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSUBB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSUBD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSUBQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSUBSB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSUBSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSUBUSB();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSUBUSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PSUBW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PTEST();
    [[nodiscard]] InterruptRaisedOr<void> handle_PTWRITE();
    [[nodiscard]] InterruptRaisedOr<void> handle_PUNPCKHBW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PUNPCKHDQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PUNPCKHQDQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PUNPCKHWD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PUNPCKLBW();
    [[nodiscard]] InterruptRaisedOr<void> handle_PUNPCKLDQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PUNPCKLQDQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PUNPCKLWD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PUSH();
    [[nodiscard]] InterruptRaisedOr<void> handle_PUSHA();
    [[nodiscard]] InterruptRaisedOr<void> handle_PUSHAD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PUSHF();
    [[nodiscard]] InterruptRaisedOr<void> handle_PUSHFD();
    [[nodiscard]] InterruptRaisedOr<void> handle_PUSHFQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_PXOR();
    [[nodiscard]] InterruptRaisedOr<void> handle_RCL();
    [[nodiscard]] InterruptRaisedOr<void> handle_RCPPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_RCPSS();
    [[nodiscard]] InterruptRaisedOr<void> handle_RCR();
    [[nodiscard]] InterruptRaisedOr<void> handle_RDFSBASE();
    [[nodiscard]] InterruptRaisedOr<void> handle_RDGSBASE();
    [[nodiscard]] InterruptRaisedOr<void> handle_RDMSR();
    [[nodiscard]] InterruptRaisedOr<void> handle_RDPID();
    [[nodiscard]] InterruptRaisedOr<void> handle_RDPKRU();
    [[nodiscard]] InterruptRaisedOr<void> handle_RDPMC();
    [[nodiscard]] InterruptRaisedOr<void> handle_RDRAND();
    [[nodiscard]] InterruptRaisedOr<void> handle_RDSEED();
    [[nodiscard]] InterruptRaisedOr<void> handle_RDSSPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_RDSSPQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_RDTSC();
    [[nodiscard]] InterruptRaisedOr<void> handle_RDTSCP();
    [[nodiscard]] InterruptRaisedOr<void> handle_REP();
    [[nodiscard]] InterruptRaisedOr<void> handle_REPE();
    [[nodiscard]] InterruptRaisedOr<void> handle_REPNE();
    [[nodiscard]] InterruptRaisedOr<void> handle_REPNZ();
    [[nodiscard]] InterruptRaisedOr<void> handle_REPZ();
    [[nodiscard]] InterruptRaisedOr<void> handle_RET();
    [[nodiscard]] InterruptRaisedOr<void> handle_ROL();
    [[nodiscard]] InterruptRaisedOr<void> handle_ROR();
    [[nodiscard]] InterruptRaisedOr<void> handle_RORX();
    [[nodiscard]] InterruptRaisedOr<void> handle_ROUNDPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_ROUNDPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_ROUNDSD();
    [[nodiscard]] InterruptRaisedOr<void> handle_ROUNDSS();
    [[nodiscard]] InterruptRaisedOr<void> handle_RSM();
    [[nodiscard]] InterruptRaisedOr<void> handle_RSQRTPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_RSQRTSS();
    [[nodiscard]] InterruptRaisedOr<void> handle_RSTORSSP();
    [[nodiscard]] InterruptRaisedOr<void> handle_SAHF();
    [[nodiscard]] InterruptRaisedOr<void> handle_SAL();
    [[nodiscard]] InterruptRaisedOr<void> handle_SAR();
    [[nodiscard]] InterruptRaisedOr<void> handle_SARX();
    [[nodiscard]] InterruptRaisedOr<void> handle_SAVEPREVSSP();
    [[nodiscard]] InterruptRaisedOr<void> handle_SBB();
    [[nodiscard]] InterruptRaisedOr<void> handle_SCAS();
    [[nodiscard]] InterruptRaisedOr<void> handle_SCASB();
    [[nodiscard]] InterruptRaisedOr<void> handle_SCASD();
    [[nodiscard]] InterruptRaisedOr<void> handle_SCASW();
    [[nodiscard]] InterruptRaisedOr<void> handle_SENDUIPI();
    [[nodiscard]] InterruptRaisedOr<void> handle_SERIALIZE();
    [[nodiscard]] InterruptRaisedOr<void> handle_SETSSBSY();
    [[nodiscard]] InterruptRaisedOr<void> handle_SETcc();
    [[nodiscard]] InterruptRaisedOr<void> handle_SFENCE();
    [[nodiscard]] InterruptRaisedOr<void> handle_SGDT();
    [[nodiscard]] InterruptRaisedOr<void> handle_SHA1MSG1();
    [[nodiscard]] InterruptRaisedOr<void> handle_SHA1MSG2();
    [[nodiscard]] InterruptRaisedOr<void> handle_SHA1NEXTE();
    [[nodiscard]] InterruptRaisedOr<void> handle_SHA1RNDS4();
    [[nodiscard]] InterruptRaisedOr<void> handle_SHA256MSG1();
    [[nodiscard]] InterruptRaisedOr<void> handle_SHA256MSG2();
    [[nodiscard]] InterruptRaisedOr<void> handle_SHA256RNDS2();
    [[nodiscard]] InterruptRaisedOr<void> handle_SHL();
    [[nodiscard]] InterruptRaisedOr<void> handle_SHLD();
    [[nodiscard]] InterruptRaisedOr<void> handle_SHLX();
    [[nodiscard]] InterruptRaisedOr<void> handle_SHR();
    [[nodiscard]] InterruptRaisedOr<void> handle_SHRD();
    [[nodiscard]] InterruptRaisedOr<void> handle_SHRX();
    [[nodiscard]] InterruptRaisedOr<void> handle_SHUFPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_SHUFPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_SIDT();
    [[nodiscard]] InterruptRaisedOr<void> handle_SLDT();
    [[nodiscard]] InterruptRaisedOr<void> handle_SMSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_SQRTPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_SQRTPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_SQRTSD();
    [[nodiscard]] InterruptRaisedOr<void> handle_SQRTSS();
    [[nodiscard]] InterruptRaisedOr<void> handle_STAC();
    [[nodiscard]] InterruptRaisedOr<void> handle_STC();
    [[nodiscard]] InterruptRaisedOr<void> handle_STD();
    [[nodiscard]] InterruptRaisedOr<void> handle_STI();
    [[nodiscard]] InterruptRaisedOr<void> handle_STMXCSR();
    [[nodiscard]] InterruptRaisedOr<void> handle_STOS();
    [[nodiscard]] InterruptRaisedOr<void> handle_STOSB();
    [[nodiscard]] InterruptRaisedOr<void> handle_STOSD();
    [[nodiscard]] InterruptRaisedOr<void> handle_STOSQ();
    [[nodiscard]] InterruptRaisedOr<void> handle_STOSW();
    [[nodiscard]] InterruptRaisedOr<void> handle_STR();
    [[nodiscard]] InterruptRaisedOr<void> handle_STTILECFG();
    [[nodiscard]] InterruptRaisedOr<void> handle_STUI();
    [[nodiscard]] InterruptRaisedOr<void> handle_SUB();
    [[nodiscard]] InterruptRaisedOr<void> handle_SUBPD();
    [[nodiscard]] InterruptRaisedOr<void> handle_SUBPS();
    [[nodiscard]] InterruptRaisedOr<void> handle_SUBSD();
    [[nodiscard]] InterruptRaisedOr<void> handle_SUBSS();
    [[nodiscard]] InterruptRaisedOr<void> handle_SWAPGS();
    [[nodiscard]] InterruptRaisedOr<void> handle_SYSCALL();
    [[nodiscard]] InterruptRaisedOr<void> handle_SYSENTER();
    [[nodiscard]] InterruptRaisedOr<void> handle_SYSEXIT();
    [[nodiscard]] InterruptRaisedOr<void> handle_SYSRET();
    [[nodiscard]] InterruptRaisedOr<void> handle_TDPBF16PS();
    [[nodiscard]] InterruptRaisedOr<void> handle_TDPBSSD();
    [[nodiscard]] InterruptRaisedOr<void> handle_TDPBSUD();
    [[nodiscard]] InterruptRaisedOr<void> handle_TDPBUSD();
    [[nodiscard]] InterruptRaisedOr<void> handle_TDPBUUD();
    [[nodiscard]] InterruptRaisedOr<void> handle_TEST();
    [[nodiscard]] InterruptRaisedOr<void> handle_TESTUI();
    [[nodiscard]] InterruptRaisedOr<void> handle_TILELOADD();
    [[nodiscard]] InterruptRaisedOr<void> handle_TILERELEASE();
    [[nodiscard]] InterruptRaisedOr<void> handle_TILESTORED();
    [[nodiscard]] InterruptRaisedOr<void> handle_TILEZERO();
    [[nodiscard]] InterruptRaisedOr<void> handle_TPAUSE();
    [[nodiscard]] InterruptRaisedOr<void> handle_TZCNT();
    [[nodiscard]] InterruptRaisedOr<void> handle_UCOMISD();
    [[nodiscard]] InterruptRaisedOr<void> handle_UCOMISS();
    [[nodiscard]] InterruptRaisedOr<void> handle_UD();
    [[nodiscard]] InterruptRaisedOr<void> handle_UIRET();

private:
    MMU m_mmu;
    PIC m_pic;
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