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
    InterruptRaisedOr<void> handle_MOV();
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
    InterruptRaisedOr<void> handle_MOVQ();
    InterruptRaisedOr<void> handle_MOVQ2DQ();
    InterruptRaisedOr<void> handle_MOVS();
    InterruptRaisedOr<void> handle_MOVSB();
    InterruptRaisedOr<void> handle_MOVSD();
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