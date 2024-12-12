
#include "cpu.h"

namespace CPUE {

// Access the detailed operand information
//cs_x86 *x86 = &(insn[i].detail->x86);

InterruptRaisedOr<void> CPU::handle_insn(cs_insn const& insn) {
    auto* detail = &(insn.detail->x86);
#define CASE(name) \
    case x86_insn::X86_INS_##name: return handle_##name(*detail); break;

    switch (insn.id) {
        CASE(ADD)
        CASE(SUB)
        CASE(MUL)
        CASE(DIV)

        CASE(MOV)
        CASE(LEA)
        //CASE(XOR)
    }
#undef CASE
}

/*void CPU::handle_RFLAGS(u64 *dest, u64 *src, cs_x86 const& insn_detail) {*/
/*    switch (insn_detail.operands[0].type) {*/
/*        case x86_op_type::X86_OP_INVALID:*/
/*            fail("Instruction is invalid!");*/
/*            break;*/
/**/
/*        case x86_op_type::X86_OP_REG:*/
/**/
/**/
/*            /**/
/*             * Only for adding*/
/*             * TODO: pack into separate functions check_of, ...*/
/*             */
/**/
/*            if ( dest[0] > std::numeric_limits<int8_t>::max() && \*/
/*                dest[0] >= 0 && src[0] >= 0)*/
/*                set_of(true);*/
/*            else*/
/*                set_of(false);*/
/**/
/*            if (dest[0] > std::numeric_limits<u64>::max())*/
/*                set_cf(true);*/
/*            else */
/*                set_cf(false);*/
/*            break;*/
/**/
/*        case x86_op_type::X86_OP_IMM:*/
/*            TODO();*/
/*            break;*/
/**/
/*        case x86_op_type::X86_OP_MEM:*/
/*            TODO();*/
/*            break;*/
/*    }*/
/*};*/

//RFLAGS: OF, CF
InterruptRaisedOr<void> CPU::handle_ADD(cs_x86 const& insn_detail) {
    if (insn_detail.operands[0].type == x86_op_type::X86_OP_INVALID)
        fail("Instruction is invalid!");
    auto first_op = insn_detail.operands[0];
    auto second_op = insn_detail.operands[1];

    if (first_op.type == x86_op_type::X86_OP_REG) {
        // TODO: check : second operand has to be REG
        auto dest_reg = m_reg64_table[first_op.reg];
        auto src_reg = m_reg64_table[second_op.reg];
        auto res = CPUE_checked_single_uadd(*dest_reg, *src_reg);
        update_rflags(res);
        *dest_reg = res.value;
    }

    if (first_op.type == x86_op_type::X86_OP_MEM) {
        // MAY_HAVE_RAISED(m_mmu.mem_read16());
        TODO();
    }

    if (first_op.type == x86_op_type::X86_OP_IMM) {
        TODO();
    }

} //	Add

InterruptRaisedOr<void> CPU::handle_AAA(cs_x86 const& insn_detail) {
    TODO();
} //	ASCII Adjust After Addition
InterruptRaisedOr<void> CPU::handle_AAD(cs_x86 const& insn_detail) {} //	ASCII Adjust AX Before Division
InterruptRaisedOr<void> CPU::handle_AAM(cs_x86 const& insn_detail) {
    TODO();
} //	ASCII Adjust AX After Multiply
InterruptRaisedOr<void> CPU::handle_AAS(cs_x86 const& insn_detail) {
    TODO();
} //	ASCII Adjust AL After Subtraction
InterruptRaisedOr<void> CPU::handle_ADC(cs_x86 const& insn_detail) {
    TODO();
} //	Add With Carry
InterruptRaisedOr<void> CPU::handle_ADCX(cs_x86 const& insn_detail) {
    TODO();
} //	Unsigned Integer Addition of Two Operands With Carry Flag
InterruptRaisedOr<void> CPU::handle_ADDPD(cs_x86 const& insn_detail) {
    TODO();
} //	Add Packed Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_ADDPS(cs_x86 const& insn_detail) {
    TODO();
} //	Add Packed Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_ADDSD(cs_x86 const& insn_detail) {
    TODO();
} //	Add Scalar Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_ADDSS(cs_x86 const& insn_detail) {
    TODO();
} //	Add Scalar Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_ADDSUBPD(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Double Precision Floating-Point Add/Subtract
InterruptRaisedOr<void> CPU::handle_ADDSUBPS(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Single Precision Floating-Point Add/Subtract
InterruptRaisedOr<void> CPU::handle_ADOX(cs_x86 const& insn_detail) {
    TODO();
} //	Unsigned Integer Addition of Two Operands With Overflow Flag
InterruptRaisedOr<void> CPU::handle_AESDEC(cs_x86 const& insn_detail) {
    TODO();
} //	Perform One Round of an AES Decryption Flow
InterruptRaisedOr<void> CPU::handle_AESDEC128KL(cs_x86 const& insn_detail) {
    TODO();
} //	Perform Ten Rounds of AES Decryption Flow With Key Locker Using 128-BitKey
InterruptRaisedOr<void> CPU::handle_AESDEC256KL(cs_x86 const& insn_detail) {
    TODO();
} //	Perform 14 Rounds of AES Decryption Flow With Key Locker Using 256-Bit Key
InterruptRaisedOr<void> CPU::handle_AESDECLAST(cs_x86 const& insn_detail) {
    TODO();
} //	Perform Last Round of an AES Decryption Flow
InterruptRaisedOr<void> CPU::handle_AESDECWIDE128KL(cs_x86 const& insn_detail) {
    TODO();
} //	Perform Ten Rounds of AES Decryption Flow With Key Locker on 8 BlocksUsing 128-Bit Key
InterruptRaisedOr<void> CPU::handle_AESDECWIDE256KL(cs_x86 const& insn_detail) {
    TODO();
} //	Perform 14 Rounds of AES Decryption Flow With Key Locker on 8 BlocksUsing 256-Bit Key
InterruptRaisedOr<void> CPU::handle_AESENC(cs_x86 const& insn_detail) {
    TODO();
} //	Perform One Round of an AES Encryption Flow
InterruptRaisedOr<void> CPU::handle_AESENC128KL(cs_x86 const& insn_detail) {
    TODO();
} //	Perform Ten Rounds of AES Encryption Flow With Key Locker Using 128-Bit Key
InterruptRaisedOr<void> CPU::handle_AESENC256KL(cs_x86 const& insn_detail) {
    TODO();
} //	Perform 14 Rounds of AES Encryption Flow With Key Locker Using 256-Bit Key
InterruptRaisedOr<void> CPU::handle_AESENCLAST(cs_x86 const& insn_detail) {
    TODO();
} //	Perform Last Round of an AES Encryption Flow
InterruptRaisedOr<void> CPU::handle_AESENCWIDE128KL(cs_x86 const& insn_detail) {
    TODO();
} //	Perform Ten Rounds of AES Encryption Flow With Key Locker on 8 BlocksUsing 128-Bit Key
InterruptRaisedOr<void> CPU::handle_AESENCWIDE256KL(cs_x86 const& insn_detail) {
    TODO();
} //	Perform 14 Rounds of AES Encryption Flow With Key Locker on 8 BlocksUsing 256-Bit Key
InterruptRaisedOr<void> CPU::handle_AESIMC(cs_x86 const& insn_detail) {
    TODO();
} //	Perform the AES InvMixColumn Transformation
InterruptRaisedOr<void> CPU::handle_AESKEYGENASSIST(cs_x86 const& insn_detail) {
    TODO();
} //	AES Round Key Generation Assist
InterruptRaisedOr<void> CPU::handle_AND(cs_x86 const& insn_detail) {
    TODO();
} //	Logical AND
InterruptRaisedOr<void> CPU::handle_ANDN(cs_x86 const& insn_detail) {
    TODO();
} //	Logical AND NOT
InterruptRaisedOr<void> CPU::handle_ANDNPD(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical AND NOT of Packed Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_ANDNPS(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical AND NOT of Packed Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_ANDPD(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical AND of Packed Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_ANDPS(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical AND of Packed Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_ARPL(cs_x86 const& insn_detail) {
    TODO();
} //	Adjust RPL Field of Segment Selector
InterruptRaisedOr<void> CPU::handle_BEXTR(cs_x86 const& insn_detail) {
    TODO();
} //	Bit Field Extract
InterruptRaisedOr<void> CPU::handle_BLENDPD(cs_x86 const& insn_detail) {
    TODO();
} //	Blend Packed Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_BLENDPS(cs_x86 const& insn_detail) {
    TODO();
} //	Blend Packed Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_BLENDVPD(cs_x86 const& insn_detail) {
    TODO();
} //	Variable Blend Packed Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_BLENDVPS(cs_x86 const& insn_detail) {
    TODO();
} //	Variable Blend Packed Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_BLSI(cs_x86 const& insn_detail) {
    TODO();
} //	Extract Lowest Set Isolated Bit
InterruptRaisedOr<void> CPU::handle_BLSMSK(cs_x86 const& insn_detail) {
    TODO();
} //	Get Mask Up to Lowest Set Bit
InterruptRaisedOr<void> CPU::handle_BLSR(cs_x86 const& insn_detail) {
    TODO();
} //	Reset Lowest Set Bit
InterruptRaisedOr<void> CPU::handle_BNDCL(cs_x86 const& insn_detail) {
    TODO();
} //	Check Lower Bound
InterruptRaisedOr<void> CPU::handle_BNDCN(cs_x86 const& insn_detail) {
    TODO();
} //	Check Upper Bound
InterruptRaisedOr<void> CPU::handle_BNDCU(cs_x86 const& insn_detail) {
    TODO();
} //	Check Upper Bound
InterruptRaisedOr<void> CPU::handle_BNDLDX(cs_x86 const& insn_detail) {
    TODO();
} //	Load Extended Bounds Using Address Translation
InterruptRaisedOr<void> CPU::handle_BNDMK(cs_x86 const& insn_detail) {
    TODO();
} //	Make Bounds
InterruptRaisedOr<void> CPU::handle_BNDMOV(cs_x86 const& insn_detail) {
    TODO();
} //	Move Bounds
InterruptRaisedOr<void> CPU::handle_BNDSTX(cs_x86 const& insn_detail) {
    TODO();
} //	Store Extended Bounds Using Address Translation
InterruptRaisedOr<void> CPU::handle_BOUND(cs_x86 const& insn_detail) {
    TODO("Only handle interrupt if out of bounds");
    static Interrupt i = {
        .vector = 5,
        .type = InterruptType::SOFTWARE_INTERRUPT,
        .iclass = InterruptClass::BENIGN,
    };
    return handle_interrupt(i);
} //	Check Array Index Against Bounds
InterruptRaisedOr<void> CPU::handle_BSF(cs_x86 const& insn_detail) {
    TODO();
} //	Bit Scan Forward
InterruptRaisedOr<void> CPU::handle_BSR(cs_x86 const& insn_detail) {
    TODO();
} //	Bit Scan Reverse
InterruptRaisedOr<void> CPU::handle_BSWAP(cs_x86 const& insn_detail) {
    TODO();
} //	Byte Swap
InterruptRaisedOr<void> CPU::handle_BT(cs_x86 const& insn_detail) {
    TODO();
} //	Bit Test
InterruptRaisedOr<void> CPU::handle_BTC(cs_x86 const& insn_detail) {
    TODO();
} //	Bit Test and Complement
InterruptRaisedOr<void> CPU::handle_BTR(cs_x86 const& insn_detail) {
    TODO();
} //	Bit Test and Reset
InterruptRaisedOr<void> CPU::handle_BTS(cs_x86 const& insn_detail) {
    TODO();
} //	Bit Test and Set
InterruptRaisedOr<void> CPU::handle_BZHI(cs_x86 const& insn_detail) {
    TODO();
} //	Zero High Bits Starting with Specified Bit Position
InterruptRaisedOr<void> CPU::handle_CALL(cs_x86 const& insn_detail) {
    TODO();
} //	Call Procedure
InterruptRaisedOr<void> CPU::handle_CBW(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Byte to Word/Convert Word to Doubleword/Convert Doubleword toQuadword
InterruptRaisedOr<void> CPU::handle_CDQ(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Word to Doubleword/Convert Doubleword to Quadword
InterruptRaisedOr<void> CPU::handle_CDQE(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Byte to Word/Convert Word to Doubleword/Convert Doubleword toQuadword
InterruptRaisedOr<void> CPU::handle_CLAC(cs_x86 const& insn_detail) {
    TODO();
} //	Clear AC Flag in EFLAGS Register
InterruptRaisedOr<void> CPU::handle_CLC(cs_x86 const& insn_detail) {
    TODO();
} //	Clear Carry Flag
InterruptRaisedOr<void> CPU::handle_CLD(cs_x86 const& insn_detail) {
    TODO();
} //	Clear Direction Flag
InterruptRaisedOr<void> CPU::handle_CLDEMOTE(cs_x86 const& insn_detail) {
    TODO();
} //	Cache Line Demote
InterruptRaisedOr<void> CPU::handle_CLFLUSH(cs_x86 const& insn_detail) {
    TODO();
} //	Flush Cache Line
InterruptRaisedOr<void> CPU::handle_CLFLUSHOPT(cs_x86 const& insn_detail) {
    TODO();
} //	Flush Cache Line Optimized
InterruptRaisedOr<void> CPU::handle_CLI(cs_x86 const& insn_detail) {
    TODO();
} //	Clear Interrupt Flag
InterruptRaisedOr<void> CPU::handle_CLRSSBSY(cs_x86 const& insn_detail) {
    TODO();
} //	Clear Busy Flag in a Supervisor Shadow Stack Token
InterruptRaisedOr<void> CPU::handle_CLTS(cs_x86 const& insn_detail) {
    TODO();
} //	Clear Task-Switched Flag in CR0
InterruptRaisedOr<void> CPU::handle_CLUI(cs_x86 const& insn_detail) {
    TODO();
} //	Clear User Interrupt Flag
InterruptRaisedOr<void> CPU::handle_CLWB(cs_x86 const& insn_detail) {
    TODO();
} //	Cache Line Write Back
InterruptRaisedOr<void> CPU::handle_CMC(cs_x86 const& insn_detail) {
    TODO();
} //	Complement Carry Flag
InterruptRaisedOr<void> CPU::handle_CMOVcc(cs_x86 const& insn_detail) {
    TODO();
} //	Conditional Move
InterruptRaisedOr<void> CPU::handle_CMP(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Two Operands
InterruptRaisedOr<void> CPU::handle_CMPPD(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Packed Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_CMPPS(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Packed Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_CMPS(cs_x86 const& insn_detail) {
    TODO();
} //	Compare String Operands
InterruptRaisedOr<void> CPU::handle_CMPSB(cs_x86 const& insn_detail) {
    TODO();
} //	Compare String Operands
InterruptRaisedOr<void> CPU::handle_CMPSD(cs_x86 const& insn_detail) {
    TODO();
} // (1)	Compare Scalar Double Precision Floating-Point Value
InterruptRaisedOr<void> CPU::handle_CMPSQ(cs_x86 const& insn_detail) {
    TODO();
} //	Compare String Operands
InterruptRaisedOr<void> CPU::handle_CMPSS(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Scalar Single Precision Floating-Point Value
InterruptRaisedOr<void> CPU::handle_CMPSW(cs_x86 const& insn_detail) {
    TODO();
} //	Compare String Operands
InterruptRaisedOr<void> CPU::handle_CMPXCHG(cs_x86 const& insn_detail) {
    TODO();
} //	Compare and Exchange
InterruptRaisedOr<void> CPU::handle_CMPXCHG16B(cs_x86 const& insn_detail) {
    TODO();
} //	Compare and Exchange Bytes
InterruptRaisedOr<void> CPU::handle_CMPXCHG8B(cs_x86 const& insn_detail) {
    TODO();
} //	Compare and Exchange Bytes
InterruptRaisedOr<void> CPU::handle_COMISD(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Scalar Ordered Double Precision Floating-Point Values and Set EFLAGS
InterruptRaisedOr<void> CPU::handle_COMISS(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Scalar Ordered Single Precision Floating-Point Values and Set EFLAGS
InterruptRaisedOr<void> CPU::handle_CPUID(cs_x86 const& insn_detail) {
    TODO();
} //	CPU Identification
InterruptRaisedOr<void> CPU::handle_CQO(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Word to Doubleword/Convert Doubleword to Quadword
InterruptRaisedOr<void> CPU::handle_CRC32(cs_x86 const& insn_detail) {
    TODO();
} //	Accumulate CRC32 Value
InterruptRaisedOr<void> CPU::handle_CVTDQ2PD(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Packed Doubleword Integers to Packed Double Precision Floating-PointValues
InterruptRaisedOr<void> CPU::handle_CVTDQ2PS(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Packed Doubleword Integers to Packed Single Precision Floating-PointValues
InterruptRaisedOr<void> CPU::handle_CVTPD2DQ(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Packed Double Precision Floating-Point Values to Packed DoublewordIntegers
InterruptRaisedOr<void> CPU::handle_CVTPD2PI(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Packed Double Precision Floating-Point Values to Packed Dword Integers
InterruptRaisedOr<void> CPU::handle_CVTPD2PS(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Packed Double Precision Floating-Point Values to Packed Single PrecisionFloating-Point Values
InterruptRaisedOr<void> CPU::handle_CVTPI2PD(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Packed Dword Integers to Packed Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_CVTPI2PS(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Packed Dword Integers to Packed Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_CVTPS2DQ(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Packed Single Precision Floating-Point Values to Packed SignedDoubleword Integer Values
InterruptRaisedOr<void> CPU::handle_CVTPS2PD(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Packed Single Precision Floating-Point Values to Packed Double PrecisionFloating-Point Values
InterruptRaisedOr<void> CPU::handle_CVTPS2PI(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Packed Single Precision Floating-Point Values to Packed Dword Integers
InterruptRaisedOr<void> CPU::handle_CVTSD2SI(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Scalar Double Precision Floating-Point Value to Doubleword Integer
InterruptRaisedOr<void> CPU::handle_CVTSD2SS(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Scalar Double Precision Floating-Point Value to Scalar Single PrecisionFloating-Point Value
InterruptRaisedOr<void> CPU::handle_CVTSI2SD(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Doubleword Integer to Scalar Double Precision Floating-Point Value
InterruptRaisedOr<void> CPU::handle_CVTSI2SS(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Doubleword Integer to Scalar Single Precision Floating-Point Value
InterruptRaisedOr<void> CPU::handle_CVTSS2SD(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Scalar Single Precision Floating-Point Value to Scalar Double PrecisionFloating-Point Value
InterruptRaisedOr<void> CPU::handle_CVTSS2SI(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Scalar Single Precision Floating-Point Value to Doubleword Integer
InterruptRaisedOr<void> CPU::handle_CVTTPD2DQ(cs_x86 const& insn_detail) {
    TODO();
} //	Convert with Truncation Packed Double Precision Floating-Point Values toPacked Doubleword Integers
InterruptRaisedOr<void> CPU::handle_CVTTPD2PI(cs_x86 const& insn_detail) {
    TODO();
} //	Convert With Truncation Packed Double Precision Floating-Point Values to PackedDword Integers
InterruptRaisedOr<void> CPU::handle_CVTTPS2DQ(cs_x86 const& insn_detail) {
    TODO();
} //	Convert With Truncation Packed Single Precision Floating-Point Values to PackedSigned Doubleword Integer Values
InterruptRaisedOr<void> CPU::handle_CVTTPS2PI(cs_x86 const& insn_detail) {
    TODO();
} //	Convert With Truncation Packed Single Precision Floating-Point Values to PackedDword Integers
InterruptRaisedOr<void> CPU::handle_CVTTSD2SI(cs_x86 const& insn_detail) {
    TODO();
} //	Convert With Truncation Scalar Double Precision Floating-Point Value to SignedInteger
InterruptRaisedOr<void> CPU::handle_CVTTSS2SI(cs_x86 const& insn_detail) {
    TODO();
} //	Convert With Truncation Scalar Single Precision Floating-Point Value to Integer
InterruptRaisedOr<void> CPU::handle_CWD(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Word to Doubleword/Convert Doubleword to Quadword
InterruptRaisedOr<void> CPU::handle_CWDE(cs_x86 const& insn_detail) {
    TODO();
} //	Convert Byte to Word/Convert Word to Doubleword/Convert Doubleword toQuadword
InterruptRaisedOr<void> CPU::handle_DAA(cs_x86 const& insn_detail) {
    TODO();
} //	Decimal Adjust AL After Addition
InterruptRaisedOr<void> CPU::handle_DAS(cs_x86 const& insn_detail) {
    TODO();
} //	Decimal Adjust AL After Subtraction
InterruptRaisedOr<void> CPU::handle_DEC(cs_x86 const& insn_detail) {
    TODO();
} //	Decrement by 1
InterruptRaisedOr<void> CPU::handle_DIV(cs_x86 const& insn_detail) {
    TODO();
} //	Unsigned Divide
InterruptRaisedOr<void> CPU::handle_DIVPD(cs_x86 const& insn_detail) {
    TODO();
} //	Divide Packed Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_DIVPS(cs_x86 const& insn_detail) {
    TODO();
} //	Divide Packed Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_DIVSD(cs_x86 const& insn_detail) {
    TODO();
} //	Divide Scalar Double Precision Floating-Point Value
InterruptRaisedOr<void> CPU::handle_DIVSS(cs_x86 const& insn_detail) {
    TODO();
} //	Divide Scalar Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_DPPD(cs_x86 const& insn_detail) {
    TODO();
} //	Dot Product of Packed Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_DPPS(cs_x86 const& insn_detail) {
    TODO();
} //	Dot Product of Packed Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_EMMS(cs_x86 const& insn_detail) {
    TODO();
} //	Empty MMX Technology State
InterruptRaisedOr<void> CPU::handle_ENCODEKEY128(cs_x86 const& insn_detail) {
    TODO();
} //	Encode 128-Bit Key With Key Locker
InterruptRaisedOr<void> CPU::handle_ENCODEKEY256(cs_x86 const& insn_detail) {
    TODO();
} //	Encode 256-Bit Key With Key Locker
InterruptRaisedOr<void> CPU::handle_ENDBR32(cs_x86 const& insn_detail) {
    TODO();
} //	Terminate an Indirect Branch in 32-bit and Compatibility Mode
InterruptRaisedOr<void> CPU::handle_ENDBR64(cs_x86 const& insn_detail) {
    TODO();
} //	Terminate an Indirect Branch in 64-bit Mode
InterruptRaisedOr<void> CPU::handle_ENQCMD(cs_x86 const& insn_detail) {
    TODO();
} //	Enqueue Command
InterruptRaisedOr<void> CPU::handle_ENQCMDS(cs_x86 const& insn_detail) {
    TODO();
} //	Enqueue Command Supervisor
InterruptRaisedOr<void> CPU::handle_ENTER(cs_x86 const& insn_detail) {
    TODO();
} //	Make Stack Frame for Procedure Parameters
InterruptRaisedOr<void> CPU::handle_EXTRACTPS(cs_x86 const& insn_detail) {
    TODO();
} //	Extract Packed Floating-Point Values
InterruptRaisedOr<void> CPU::handle_F2XM1(cs_x86 const& insn_detail) {
    TODO();
} //	Compute 2x–1
InterruptRaisedOr<void> CPU::handle_FABS(cs_x86 const& insn_detail) {
    TODO();
} //	Absolute Value
InterruptRaisedOr<void> CPU::handle_FADD(cs_x86 const& insn_detail) {
    TODO();
} //	Add
InterruptRaisedOr<void> CPU::handle_FADDP(cs_x86 const& insn_detail) {
    TODO();
} //	Add
InterruptRaisedOr<void> CPU::handle_FBLD(cs_x86 const& insn_detail) {
    TODO();
} //	Load Binary Coded Decimal
InterruptRaisedOr<void> CPU::handle_FBSTP(cs_x86 const& insn_detail) {
    TODO();
} //	Store BCD Integer and Pop
InterruptRaisedOr<void> CPU::handle_FCHS(cs_x86 const& insn_detail) {
    TODO();
} //	Change Sign
InterruptRaisedOr<void> CPU::handle_FCLEX(cs_x86 const& insn_detail) {
    TODO();
} //	Clear Exceptions
InterruptRaisedOr<void> CPU::handle_FCMOVcc(cs_x86 const& insn_detail) {
    TODO();
} //	Floating-Point Conditional Move
InterruptRaisedOr<void> CPU::handle_FCOM(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Floating-Point Values
InterruptRaisedOr<void> CPU::handle_FCOMI(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Floating-Point Values and Set EFLAGS
InterruptRaisedOr<void> CPU::handle_FCOMIP(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Floating-Point Values and Set EFLAGS
InterruptRaisedOr<void> CPU::handle_FCOMP(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Floating-Point Values
InterruptRaisedOr<void> CPU::handle_FCOMPP(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Floating-Point Values
InterruptRaisedOr<void> CPU::handle_FCOS(cs_x86 const& insn_detail) {
    TODO();
} //	Cosine
InterruptRaisedOr<void> CPU::handle_FDECSTP(cs_x86 const& insn_detail) {
    TODO();
} //	Decrement Stack-Top Pointer
InterruptRaisedOr<void> CPU::handle_FDIV(cs_x86 const& insn_detail) {
    TODO();
} //	Divide
InterruptRaisedOr<void> CPU::handle_FDIVP(cs_x86 const& insn_detail) {
    TODO();
} //	Divide
InterruptRaisedOr<void> CPU::handle_FDIVR(cs_x86 const& insn_detail) {
    TODO();
} //	Reverse Divide
InterruptRaisedOr<void> CPU::handle_FDIVRP(cs_x86 const& insn_detail) {
    TODO();
} //	Reverse Divide
InterruptRaisedOr<void> CPU::handle_FFREE(cs_x86 const& insn_detail) {
    TODO();
} //	Free Floating-Point Register
InterruptRaisedOr<void> CPU::handle_FIADD(cs_x86 const& insn_detail) {
    TODO();
} //	Add
InterruptRaisedOr<void> CPU::handle_FICOM(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Integer
InterruptRaisedOr<void> CPU::handle_FICOMP(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Integer
InterruptRaisedOr<void> CPU::handle_FIDIV(cs_x86 const& insn_detail) {
    TODO();
} //	Divide
InterruptRaisedOr<void> CPU::handle_FIDIVR(cs_x86 const& insn_detail) {
    TODO();
} //	Reverse Divide
InterruptRaisedOr<void> CPU::handle_FILD(cs_x86 const& insn_detail) {
    TODO();
} //	Load Integer
InterruptRaisedOr<void> CPU::handle_FIMUL(cs_x86 const& insn_detail) {
    TODO();
} //	Multiply
InterruptRaisedOr<void> CPU::handle_FINCSTP(cs_x86 const& insn_detail) {
    TODO();
} //	Increment Stack-Top Pointer
InterruptRaisedOr<void> CPU::handle_FINIT(cs_x86 const& insn_detail) {
    TODO();
} //	Initialize Floating-Point Unit
InterruptRaisedOr<void> CPU::handle_FIST(cs_x86 const& insn_detail) {
    TODO();
} //	Store Integer
InterruptRaisedOr<void> CPU::handle_FISTP(cs_x86 const& insn_detail) {
    TODO();
} //	Store Integer
InterruptRaisedOr<void> CPU::handle_FISTTP(cs_x86 const& insn_detail) {
    TODO();
} //	Store Integer With Truncation
InterruptRaisedOr<void> CPU::handle_FISUB(cs_x86 const& insn_detail) {
    TODO();
} //	Subtract
InterruptRaisedOr<void> CPU::handle_FISUBR(cs_x86 const& insn_detail) {
    TODO();
} //	Reverse Subtract
InterruptRaisedOr<void> CPU::handle_FLD(cs_x86 const& insn_detail) {
    TODO();
} //	Load Floating-Point Value
InterruptRaisedOr<void> CPU::handle_FLD1(cs_x86 const& insn_detail) {
    TODO();
} //	Load Constant
InterruptRaisedOr<void> CPU::handle_FLDCW(cs_x86 const& insn_detail) {
    TODO();
} //	Load x87 FPU Control Word
InterruptRaisedOr<void> CPU::handle_FLDENV(cs_x86 const& insn_detail) {
    TODO();
} //	Load x87 FPU Environment
InterruptRaisedOr<void> CPU::handle_FLDL2E(cs_x86 const& insn_detail) {
    TODO();
} //	Load Constant
InterruptRaisedOr<void> CPU::handle_FLDL2T(cs_x86 const& insn_detail) {
    TODO();
} //	Load Constant
InterruptRaisedOr<void> CPU::handle_FLDLG2(cs_x86 const& insn_detail) {
    TODO();
} //	Load Constant
InterruptRaisedOr<void> CPU::handle_FLDLN2(cs_x86 const& insn_detail) {
    TODO();
} //	Load Constant
InterruptRaisedOr<void> CPU::handle_FLDPI(cs_x86 const& insn_detail) {
    TODO();
} //	Load Constant
InterruptRaisedOr<void> CPU::handle_FLDZ(cs_x86 const& insn_detail) {
    TODO();
} //	Load Constant
InterruptRaisedOr<void> CPU::handle_FMUL(cs_x86 const& insn_detail) {
    TODO();
} //	Multiply
InterruptRaisedOr<void> CPU::handle_FMULP(cs_x86 const& insn_detail) {
    TODO();
} //	Multiply
InterruptRaisedOr<void> CPU::handle_FNCLEX(cs_x86 const& insn_detail) {
    TODO();
} //	Clear Exceptions
InterruptRaisedOr<void> CPU::handle_FNINIT(cs_x86 const& insn_detail) {
    TODO();
} //	Initialize Floating-Point Unit
InterruptRaisedOr<void> CPU::handle_FNOP(cs_x86 const& insn_detail) {
    TODO();
} //	No Operation
InterruptRaisedOr<void> CPU::handle_FNSAVE(cs_x86 const& insn_detail) {
    TODO();
} //	Store x87 FPU State
InterruptRaisedOr<void> CPU::handle_FNSTCW(cs_x86 const& insn_detail) {
    TODO();
} //	Store x87 FPU Control Word
InterruptRaisedOr<void> CPU::handle_FNSTENV(cs_x86 const& insn_detail) {
    TODO();
} //	Store x87 FPU Environment
InterruptRaisedOr<void> CPU::handle_FNSTSW(cs_x86 const& insn_detail) {
    TODO();
} //	Store x87 FPU Status Word
InterruptRaisedOr<void> CPU::handle_FPATAN(cs_x86 const& insn_detail) {
    TODO();
} //	Partial Arctangent
InterruptRaisedOr<void> CPU::handle_FPREM(cs_x86 const& insn_detail) {
    TODO();
} //	Partial Remainder
InterruptRaisedOr<void> CPU::handle_FPREM1(cs_x86 const& insn_detail) {
    TODO();
} //	Partial Remainder
InterruptRaisedOr<void> CPU::handle_FPTAN(cs_x86 const& insn_detail) {
    TODO();
} //	Partial Tangent
InterruptRaisedOr<void> CPU::handle_FRNDINT(cs_x86 const& insn_detail) {
    TODO();
} //	Round to Integer
InterruptRaisedOr<void> CPU::handle_FRSTOR(cs_x86 const& insn_detail) {
    TODO();
} //	Restore x87 FPU State
InterruptRaisedOr<void> CPU::handle_FSAVE(cs_x86 const& insn_detail) {
    TODO();
} //	Store x87 FPU State
InterruptRaisedOr<void> CPU::handle_FSCALE(cs_x86 const& insn_detail) {
    TODO();
} //	Scale
InterruptRaisedOr<void> CPU::handle_FSIN(cs_x86 const& insn_detail) {
    TODO();
} //	Sine
InterruptRaisedOr<void> CPU::handle_FSINCOS(cs_x86 const& insn_detail) {
    TODO();
} //	Sine and Cosine
InterruptRaisedOr<void> CPU::handle_FSQRT(cs_x86 const& insn_detail) {
    TODO();
} //	Square Root
InterruptRaisedOr<void> CPU::handle_FST(cs_x86 const& insn_detail) {
    TODO();
} //	Store Floating-Point Value
InterruptRaisedOr<void> CPU::handle_FSTCW(cs_x86 const& insn_detail) {
    TODO();
} //	Store x87 FPU Control Word
InterruptRaisedOr<void> CPU::handle_FSTENV(cs_x86 const& insn_detail) {
    TODO();
} //	Store x87 FPU Environment
InterruptRaisedOr<void> CPU::handle_FSTP(cs_x86 const& insn_detail) {
    TODO();
} //	Store Floating-Point Value
InterruptRaisedOr<void> CPU::handle_FSTSW(cs_x86 const& insn_detail) {
    TODO();
} //	Store x87 FPU Status Word
InterruptRaisedOr<void> CPU::handle_FSUB(cs_x86 const& insn_detail) {
    TODO();
} //	Subtract
InterruptRaisedOr<void> CPU::handle_FSUBP(cs_x86 const& insn_detail) {
    TODO();
} //	Subtract
InterruptRaisedOr<void> CPU::handle_FSUBR(cs_x86 const& insn_detail) {
    TODO();
} //	Reverse Subtract
InterruptRaisedOr<void> CPU::handle_FSUBRP(cs_x86 const& insn_detail) {
    TODO();
} //	Reverse Subtract
InterruptRaisedOr<void> CPU::handle_FTST(cs_x86 const& insn_detail) {
    TODO();
} //	TEST
InterruptRaisedOr<void> CPU::handle_FUCOM(cs_x86 const& insn_detail) {
    TODO();
} //	Unordered Compare Floating-Point Values
InterruptRaisedOr<void> CPU::handle_FUCOMI(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Floating-Point Values and Set EFLAGS
InterruptRaisedOr<void> CPU::handle_FUCOMIP(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Floating-Point Values and Set EFLAGS
InterruptRaisedOr<void> CPU::handle_FUCOMP(cs_x86 const& insn_detail) {
    TODO();
} //	Unordered Compare Floating-Point Values
InterruptRaisedOr<void> CPU::handle_FUCOMPP(cs_x86 const& insn_detail) {
    TODO();
} //	Unordered Compare Floating-Point Values
InterruptRaisedOr<void> CPU::handle_FWAIT(cs_x86 const& insn_detail) {
    TODO();
} //	Wait
InterruptRaisedOr<void> CPU::handle_FXAM(cs_x86 const& insn_detail) {
    TODO();
} //	Examine Floating-Point
InterruptRaisedOr<void> CPU::handle_FXCH(cs_x86 const& insn_detail) {
    TODO();
} //	Exchange Register Contents
InterruptRaisedOr<void> CPU::handle_FXRSTOR(cs_x86 const& insn_detail) {
    TODO();
} //	Restore x87 FPU, MMX, XMM, and MXCSR State
InterruptRaisedOr<void> CPU::handle_FXSAVE(cs_x86 const& insn_detail) {
    TODO();
} //	Save x87 FPU, MMX Technology, and SSE State
InterruptRaisedOr<void> CPU::handle_FXTRACT(cs_x86 const& insn_detail) {
    TODO();
} //	Extract Exponent and Significand
InterruptRaisedOr<void> CPU::handle_FYL2X(cs_x86 const& insn_detail) {
    TODO();
} //	Compute y ∗ log2x
InterruptRaisedOr<void> CPU::handle_FYL2XP1(cs_x86 const& insn_detail) {
    TODO();
} //	Compute y ∗ log2(x +1)
InterruptRaisedOr<void> CPU::handle_GF2P8AFFINEINVQB(cs_x86 const& insn_detail) {
    TODO();
} //	Galois Field Affine Transformation Inverse
InterruptRaisedOr<void> CPU::handle_GF2P8AFFINEQB(cs_x86 const& insn_detail) {
    TODO();
} //	Galois Field Affine Transformation
InterruptRaisedOr<void> CPU::handle_GF2P8MULB(cs_x86 const& insn_detail) {
    TODO();
} //	Galois Field Multiply Bytes
InterruptRaisedOr<void> CPU::handle_HADDPD(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Double Precision Floating-Point Horizontal Add
InterruptRaisedOr<void> CPU::handle_HADDPS(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Single Precision Floating-Point Horizontal Add
InterruptRaisedOr<void> CPU::handle_HLT(cs_x86 const& insn_detail) {
    TODO();
} //	Halt
InterruptRaisedOr<void> CPU::handle_HRESET(cs_x86 const& insn_detail) {
    TODO();
} //	History Reset
InterruptRaisedOr<void> CPU::handle_HSUBPD(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Double Precision Floating-Point Horizontal Subtract
InterruptRaisedOr<void> CPU::handle_HSUBPS(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Single Precision Floating-Point Horizontal Subtract
InterruptRaisedOr<void> CPU::handle_IDIV(cs_x86 const& insn_detail) {
    TODO();
} //	Signed Divide
InterruptRaisedOr<void> CPU::handle_IMUL(cs_x86 const& insn_detail) {
    TODO();
} //	Signed Multiply
InterruptRaisedOr<void> CPU::handle_IN(cs_x86 const& insn_detail) {
    TODO();
} //	Input From Port
InterruptRaisedOr<void> CPU::handle_INC(cs_x86 const& insn_detail) {
    TODO();
} //	Increment by 1
InterruptRaisedOr<void> CPU::handle_INCSSPD(cs_x86 const& insn_detail) {
    TODO();
} //	Increment Shadow Stack Pointer
InterruptRaisedOr<void> CPU::handle_INCSSPQ(cs_x86 const& insn_detail) {
    TODO();
} //	Increment Shadow Stack Pointer
InterruptRaisedOr<void> CPU::handle_INS(cs_x86 const& insn_detail) {
    TODO();
} //	Input from Port to String
InterruptRaisedOr<void> CPU::handle_INSB(cs_x86 const& insn_detail) {
    TODO();
} //	Input from Port to String
InterruptRaisedOr<void> CPU::handle_INSD(cs_x86 const& insn_detail) {
    TODO();
} //	Input from Port to String
InterruptRaisedOr<void> CPU::handle_INSERTPS(cs_x86 const& insn_detail) {
    TODO();
} //	Insert Scalar Single Precision Floating-Point Value
InterruptRaisedOr<void> CPU::handle_INSW(cs_x86 const& insn_detail) {
    TODO();
} //	Input from Port to String
InterruptRaisedOr<void> CPU::handle_INT(cs_x86 const& insn_detail) {
    // TODO: increment m_rip, because the return address is the next insn
    Interrupt i = {
        .vector = 0, // TODO: use actual vector
        .type = InterruptType::SOFTWARE_INTERRUPT,
        .iclass = InterruptClass::BENIGN,
        .source = InterruptSource::INTN_INT3_INTO_INSN,
    };
    return handle_interrupt(i);
} // 	Call to Interrupt Procedure
InterruptRaisedOr<void> CPU::handle_INT1(cs_x86 const& insn_detail) {
    static Interrupt i = {
        .vector = 1,
        .type = InterruptType::SOFTWARE_INTERRUPT,
        .iclass = InterruptClass::BENIGN,
        .source = InterruptSource::INT1_INSN,
    };
    return handle_interrupt(i);
} //	Call to Interrupt Procedure
InterruptRaisedOr<void> CPU::handle_INT3(cs_x86 const& insn_detail) {
    TODO_NOFAIL("increment rip");
    static Interrupt i = {
        .vector = 3,
        .type = InterruptType::SOFTWARE_INTERRUPT,
        .iclass = InterruptClass::BENIGN,
        .source = InterruptSource::INTN_INT3_INTO_INSN,
    };
    return handle_interrupt(i);
} //	Call to Interrupt Procedure
InterruptRaisedOr<void> CPU::handle_INTO(cs_x86 const& insn_detail) {
    TODO_NOFAIL("increment rip");
    static Interrupt i = {
        .vector = 4,
        .type = InterruptType::SOFTWARE_INTERRUPT,
        .iclass = InterruptClass::BENIGN,
        .source = InterruptSource::INTN_INT3_INTO_INSN,
    };
    if (m_rflags.OF) {
        return handle_interrupt(i);
    }
    return {};
} //	Call to Interrupt Procedure
InterruptRaisedOr<void> CPU::handle_INVD(cs_x86 const& insn_detail) {
    TODO();
} //	Invalidate Internal Caches
InterruptRaisedOr<void> CPU::handle_INVLPG(cs_x86 const& insn_detail) {
    TODO();
} //	Invalidate TLB Entries
InterruptRaisedOr<void> CPU::handle_INVPCID(cs_x86 const& insn_detail) {
    TODO();
} //	Invalidate Process-Context Identifier
InterruptRaisedOr<void> CPU::handle_IRET(cs_x86 const& insn_detail) {
    TODO();
} //	Interrupt Return
InterruptRaisedOr<void> CPU::handle_IRETD(cs_x86 const& insn_detail) {
    TODO();
} //	Interrupt Return
InterruptRaisedOr<void> CPU::handle_IRETQ(cs_x86 const& insn_detail) {
    TODO();
} //	Interrupt Return
InterruptRaisedOr<void> CPU::handle_JMP(cs_x86 const& insn_detail) {
    TODO();
} //	Jump
InterruptRaisedOr<void> CPU::handle_Jcc(cs_x86 const& insn_detail) {
    TODO();
} //	Jump if Condition Is Met
InterruptRaisedOr<void> CPU::handle_KADDB(cs_x86 const& insn_detail) {
    TODO();
} //	ADD Two Masks
InterruptRaisedOr<void> CPU::handle_KADDD(cs_x86 const& insn_detail) {
    TODO();
} //	ADD Two Masks
InterruptRaisedOr<void> CPU::handle_KADDQ(cs_x86 const& insn_detail) {
    TODO();
} //	ADD Two Masks
InterruptRaisedOr<void> CPU::handle_KADDW(cs_x86 const& insn_detail) {
    TODO();
} //	ADD Two Masks
InterruptRaisedOr<void> CPU::handle_KANDB(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical AND Masks
InterruptRaisedOr<void> CPU::handle_KANDD(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical AND Masks
InterruptRaisedOr<void> CPU::handle_KANDNB(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical AND NOT Masks
InterruptRaisedOr<void> CPU::handle_KANDND(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical AND NOT Masks
InterruptRaisedOr<void> CPU::handle_KANDNQ(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical AND NOT Masks
InterruptRaisedOr<void> CPU::handle_KANDNW(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical AND NOT Masks
InterruptRaisedOr<void> CPU::handle_KANDQ(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical AND Masks
InterruptRaisedOr<void> CPU::handle_KANDW(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical AND Masks
InterruptRaisedOr<void> CPU::handle_KMOVB(cs_x86 const& insn_detail) {
    TODO();
} //	Move From and to Mask Registers
InterruptRaisedOr<void> CPU::handle_KMOVD(cs_x86 const& insn_detail) {
    TODO();
} //	Move From and to Mask Registers
InterruptRaisedOr<void> CPU::handle_KMOVQ(cs_x86 const& insn_detail) {
    TODO();
} //	Move From and to Mask Registers
InterruptRaisedOr<void> CPU::handle_KMOVW(cs_x86 const& insn_detail) {
    TODO();
} //	Move From and to Mask Registers
InterruptRaisedOr<void> CPU::handle_KNOTB(cs_x86 const& insn_detail) {
    TODO();
} //	NOT Mask Register
InterruptRaisedOr<void> CPU::handle_KNOTD(cs_x86 const& insn_detail) {
    TODO();
} //	NOT Mask Register
InterruptRaisedOr<void> CPU::handle_KNOTQ(cs_x86 const& insn_detail) {
    TODO();
} //	NOT Mask Register
InterruptRaisedOr<void> CPU::handle_KNOTW(cs_x86 const& insn_detail) {
    TODO();
} //	NOT Mask Register
InterruptRaisedOr<void> CPU::handle_KORB(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical OR Masks
InterruptRaisedOr<void> CPU::handle_KORD(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical OR Masks
InterruptRaisedOr<void> CPU::handle_KORQ(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical OR Masks
InterruptRaisedOr<void> CPU::handle_KORTESTB(cs_x86 const& insn_detail) {
    TODO();
} //	OR Masks and Set Flags
InterruptRaisedOr<void> CPU::handle_KORTESTD(cs_x86 const& insn_detail) {
    TODO();
} //	OR Masks and Set Flags
InterruptRaisedOr<void> CPU::handle_KORTESTQ(cs_x86 const& insn_detail) {
    TODO();
} //	OR Masks and Set Flags
InterruptRaisedOr<void> CPU::handle_KORTESTW(cs_x86 const& insn_detail) {
    TODO();
} //	OR Masks and Set Flags
InterruptRaisedOr<void> CPU::handle_KORW(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical OR Masks
InterruptRaisedOr<void> CPU::handle_KSHIFTLB(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Left Mask Registers
InterruptRaisedOr<void> CPU::handle_KSHIFTLD(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Left Mask Registers
InterruptRaisedOr<void> CPU::handle_KSHIFTLQ(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Left Mask Registers
InterruptRaisedOr<void> CPU::handle_KSHIFTLW(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Left Mask Registers
InterruptRaisedOr<void> CPU::handle_KSHIFTRB(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Right Mask Registers
InterruptRaisedOr<void> CPU::handle_KSHIFTRD(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Right Mask Registers
InterruptRaisedOr<void> CPU::handle_KSHIFTRQ(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Right Mask Registers
InterruptRaisedOr<void> CPU::handle_KSHIFTRW(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Right Mask Registers
InterruptRaisedOr<void> CPU::handle_KTESTB(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Bit Test Masks and Set Flags
InterruptRaisedOr<void> CPU::handle_KTESTD(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Bit Test Masks and Set Flags
InterruptRaisedOr<void> CPU::handle_KTESTQ(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Bit Test Masks and Set Flags
InterruptRaisedOr<void> CPU::handle_KTESTW(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Bit Test Masks and Set Flags
InterruptRaisedOr<void> CPU::handle_KUNPCKBW(cs_x86 const& insn_detail) {
    TODO();
} //	Unpack for Mask Registers
InterruptRaisedOr<void> CPU::handle_KUNPCKDQ(cs_x86 const& insn_detail) {
    TODO();
} //	Unpack for Mask Registers
InterruptRaisedOr<void> CPU::handle_KUNPCKWD(cs_x86 const& insn_detail) {
    TODO();
} //	Unpack for Mask Registers
InterruptRaisedOr<void> CPU::handle_KXNORB(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical XNOR Masks
InterruptRaisedOr<void> CPU::handle_KXNORD(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical XNOR Masks
InterruptRaisedOr<void> CPU::handle_KXNORQ(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical XNOR Masks
InterruptRaisedOr<void> CPU::handle_KXNORW(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical XNOR Masks
InterruptRaisedOr<void> CPU::handle_KXORB(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical XOR Masks
InterruptRaisedOr<void> CPU::handle_KXORD(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical XOR Masks
InterruptRaisedOr<void> CPU::handle_KXORQ(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical XOR Masks
InterruptRaisedOr<void> CPU::handle_KXORW(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical XOR Masks
InterruptRaisedOr<void> CPU::handle_LAHF(cs_x86 const& insn_detail) {
    TODO();
} //	Load Status Flags Into AH Register
InterruptRaisedOr<void> CPU::handle_LAR(cs_x86 const& insn_detail) {
    TODO();
} //	Load Access Rights Byte
InterruptRaisedOr<void> CPU::handle_LDDQU(cs_x86 const& insn_detail) {
    TODO();
} //	Load Unaligned Integer 128 Bits
InterruptRaisedOr<void> CPU::handle_LDMXCSR(cs_x86 const& insn_detail) {
    TODO();
} //	Load MXCSR Register
InterruptRaisedOr<void> CPU::handle_LDS(cs_x86 const& insn_detail) {
    TODO();
} //	Load Far Pointer
InterruptRaisedOr<void> CPU::handle_LDTILECFG(cs_x86 const& insn_detail) {
    TODO();
} //	Load Tile Configuration
InterruptRaisedOr<void> CPU::handle_LEA(cs_x86 const& insn_detail) {
    TODO();
} //	Load Effective Address
InterruptRaisedOr<void> CPU::handle_LEAVE(cs_x86 const& insn_detail) {
    TODO();
} //	High Level Procedure Exit
InterruptRaisedOr<void> CPU::handle_LES(cs_x86 const& insn_detail) {
    TODO();
} //	Load Far Pointer
InterruptRaisedOr<void> CPU::handle_LFENCE(cs_x86 const& insn_detail) {
    TODO();
} //	Load Fence
InterruptRaisedOr<void> CPU::handle_LFS(cs_x86 const& insn_detail) {
    TODO();
} //	Load Far Pointer
InterruptRaisedOr<void> CPU::handle_LGDT(cs_x86 const& insn_detail) {
    TODO();
} //	Load Global/Interrupt Descriptor Table Register
InterruptRaisedOr<void> CPU::handle_LGS(cs_x86 const& insn_detail) {
    TODO();
} //	Load Far Pointer
InterruptRaisedOr<void> CPU::handle_LIDT(cs_x86 const& insn_detail) {
    TODO();
} //	Load Global/Interrupt Descriptor Table Register
InterruptRaisedOr<void> CPU::handle_LLDT(cs_x86 const& insn_detail) {
    TODO();
} //	Load Local Descriptor Table Register
InterruptRaisedOr<void> CPU::handle_LMSW(cs_x86 const& insn_detail) {
    TODO();
} //	Load Machine Status Word
InterruptRaisedOr<void> CPU::handle_LOADIWKEY(cs_x86 const& insn_detail) {
    TODO();
} //	Load Internal Wrapping Key With Key Locker
InterruptRaisedOr<void> CPU::handle_LOCK(cs_x86 const& insn_detail) {
    TODO();
} //	Assert LOCK# Signal Prefix
InterruptRaisedOr<void> CPU::handle_LODS(cs_x86 const& insn_detail) {
    TODO();
} //	Load String
InterruptRaisedOr<void> CPU::handle_LODSB(cs_x86 const& insn_detail) {
    TODO();
} //	Load String
InterruptRaisedOr<void> CPU::handle_LODSD(cs_x86 const& insn_detail) {
    TODO();
} //	Load String
InterruptRaisedOr<void> CPU::handle_LODSQ(cs_x86 const& insn_detail) {
    TODO();
} //	Load String
InterruptRaisedOr<void> CPU::handle_LODSW(cs_x86 const& insn_detail) {
    TODO();
} //	Load String
InterruptRaisedOr<void> CPU::handle_LOOP(cs_x86 const& insn_detail) {
    TODO();
} //	Loop According to ECX Counter
InterruptRaisedOr<void> CPU::handle_LOOPcc(cs_x86 const& insn_detail) {
    TODO();
} //	Loop According to ECX Counter
InterruptRaisedOr<void> CPU::handle_LSL(cs_x86 const& insn_detail) {
    TODO();
} //	Load Segment Limit
InterruptRaisedOr<void> CPU::handle_LSS(cs_x86 const& insn_detail) {
    TODO();
} //	Load Far Pointer
InterruptRaisedOr<void> CPU::handle_LTR(cs_x86 const& insn_detail) {
    TODO();
} //	Load Task Register
InterruptRaisedOr<void> CPU::handle_LZCNT(cs_x86 const& insn_detail) {
    TODO();
} //	Count the Number of Leading Zero Bits
InterruptRaisedOr<void> CPU::handle_MASKMOVDQU(cs_x86 const& insn_detail) {
    TODO();
} //	Store Selected Bytes of Double Quadword
InterruptRaisedOr<void> CPU::handle_MASKMOVQ(cs_x86 const& insn_detail) {
    TODO();
} //	Store Selected Bytes of Quadword
InterruptRaisedOr<void> CPU::handle_MAXPD(cs_x86 const& insn_detail) {
    TODO();
} //	Maximum of Packed Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_MAXPS(cs_x86 const& insn_detail) {
    TODO();
} //	Maximum of Packed Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_MAXSD(cs_x86 const& insn_detail) {
    TODO();
} //	Return Maximum Scalar Double Precision Floating-Point Value
InterruptRaisedOr<void> CPU::handle_MAXSS(cs_x86 const& insn_detail) {
    TODO();
} //	Return Maximum Scalar Single Precision Floating-Point Value
InterruptRaisedOr<void> CPU::handle_MFENCE(cs_x86 const& insn_detail) {
    TODO();
} //	Memory Fence
InterruptRaisedOr<void> CPU::handle_MINPD(cs_x86 const& insn_detail) {
    TODO();
} //	Minimum of Packed Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_MINPS(cs_x86 const& insn_detail) {
    TODO();
} //	Minimum of Packed Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_MINSD(cs_x86 const& insn_detail) {
    TODO();
} //	Return Minimum Scalar Double Precision Floating-Point Value
InterruptRaisedOr<void> CPU::handle_MINSS(cs_x86 const& insn_detail) {
    TODO();
} //	Return Minimum Scalar Single Precision Floating-Point Value
InterruptRaisedOr<void> CPU::handle_MONITOR(cs_x86 const& insn_detail) {
    TODO();
} //	Set Up Monitor Address
InterruptRaisedOr<void> CPU::handle_MOV(cs_x86 const& insn_detail) {
    /**
     * TODO:
     * Attempting to set any reserved bits
     * in CR0[31:0] is ignored. Attempting to set any reserved bits in CR0[63:32] results in a general-protection excep-
     * tion, #GP(0). When PCIDs are not enabled, bits 2:0 and bits 11:5 of CR3 are not used and attempts to set them
     * are ignored. Attempting to set any reserved bits in CR3[63:MAXPHYADDR] results in #GP(0). Attempting to set
     * any reserved bits in CR4 results in #GP(0).
     * Which reserved bit violations generate #GP(0) additionally:
     * Attempting to write a non-zero value into the reserved bits of the MXCSR register.
     * Writing to a reserved bit in an MSR.
     * If an attempt is made to set a reserved bit in CR3, CR4 or CR8.
     */
    TODO();
} //	Move
InterruptRaisedOr<void> CPU::handle_MOVAPD(cs_x86 const& insn_detail) {
    TODO();
} //	Move Aligned Packed Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_MOVAPS(cs_x86 const& insn_detail) {
    TODO();
} //	Move Aligned Packed Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_MOVBE(cs_x86 const& insn_detail) {
    TODO();
} //	Move Data After Swapping Bytes
InterruptRaisedOr<void> CPU::handle_MOVD(cs_x86 const& insn_detail) {
    TODO();
} //	Move Doubleword/Move Quadword
InterruptRaisedOr<void> CPU::handle_MOVDDUP(cs_x86 const& insn_detail) {
    TODO();
} //	Replicate Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_MOVDIR64B(cs_x86 const& insn_detail) {
    TODO();
} //	Move 64 Bytes as Direct Store
InterruptRaisedOr<void> CPU::handle_MOVDIRI(cs_x86 const& insn_detail) {
    TODO();
} //	Move Doubleword as Direct Store
InterruptRaisedOr<void> CPU::handle_MOVDQ2Q(cs_x86 const& insn_detail) {
    TODO();
} //	Move Quadword from XMM to MMX Technology Register
InterruptRaisedOr<void> CPU::handle_MOVDQA(cs_x86 const& insn_detail) {
    TODO();
} //	Move Aligned Packed Integer Values
InterruptRaisedOr<void> CPU::handle_MOVDQU(cs_x86 const& insn_detail) {
    TODO();
} //	Move Unaligned Packed Integer Values
InterruptRaisedOr<void> CPU::handle_MOVHLPS(cs_x86 const& insn_detail) {
    TODO();
} //	Move Packed Single Precision Floating-Point Values High to Low
InterruptRaisedOr<void> CPU::handle_MOVHPD(cs_x86 const& insn_detail) {
    TODO();
} //	Move High Packed Double Precision Floating-Point Value
InterruptRaisedOr<void> CPU::handle_MOVHPS(cs_x86 const& insn_detail) {
    TODO();
} //	Move High Packed Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_MOVLHPS(cs_x86 const& insn_detail) {
    TODO();
} //	Move Packed Single Precision Floating-Point Values Low to High
InterruptRaisedOr<void> CPU::handle_MOVLPD(cs_x86 const& insn_detail) {
    TODO();
} //	Move Low Packed Double Precision Floating-Point Value
InterruptRaisedOr<void> CPU::handle_MOVLPS(cs_x86 const& insn_detail) {
    TODO();
} //	Move Low Packed Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_MOVMSKPD(cs_x86 const& insn_detail) {
    TODO();
} //	Extract Packed Double Precision Floating-Point Sign Mask
InterruptRaisedOr<void> CPU::handle_MOVMSKPS(cs_x86 const& insn_detail) {
    TODO();
} //	Extract Packed Single Precision Floating-Point Sign Mask
InterruptRaisedOr<void> CPU::handle_MOVNTDQ(cs_x86 const& insn_detail) {
    TODO();
} //	Store Packed Integers Using Non-Temporal Hint
InterruptRaisedOr<void> CPU::handle_MOVNTDQA(cs_x86 const& insn_detail) {
    TODO();
} //	Load Double Quadword Non-Temporal Aligned Hint
InterruptRaisedOr<void> CPU::handle_MOVNTI(cs_x86 const& insn_detail) {
    TODO();
} //	Store Doubleword Using Non-Temporal Hint
InterruptRaisedOr<void> CPU::handle_MOVNTPD(cs_x86 const& insn_detail) {
    TODO();
} //	Store Packed Double Precision Floating-Point Values Using Non-Temporal Hint
InterruptRaisedOr<void> CPU::handle_MOVNTPS(cs_x86 const& insn_detail) {
    TODO();
} //	Store Packed Single Precision Floating-Point Values Using Non-Temporal Hint
InterruptRaisedOr<void> CPU::handle_MOVNTQ(cs_x86 const& insn_detail) {
    TODO();
} //	Store of Quadword Using Non-Temporal Hint
InterruptRaisedOr<void> CPU::handle_MOVQ(cs_x86 const& insn_detail) {
    TODO();
} //	Move Doubleword/Move Quadword
InterruptRaisedOr<void> CPU::handle_MOVQ2DQ(cs_x86 const& insn_detail) {
    TODO();
} //	Move Quadword from MMX Technology to XMM Register
InterruptRaisedOr<void> CPU::handle_MOVS(cs_x86 const& insn_detail) {
    TODO();
} //	Move Data From String to String
InterruptRaisedOr<void> CPU::handle_MOVSB(cs_x86 const& insn_detail) {
    TODO();
} //	Move Data From String to String
InterruptRaisedOr<void> CPU::handle_MOVSD(cs_x86 const& insn_detail) {
    TODO();
} //	Move Data From String to String
InterruptRaisedOr<void> CPU::handle_MOVSHDUP(cs_x86 const& insn_detail) {
    TODO();
} //	Replicate Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_MOVSLDUP(cs_x86 const& insn_detail) {
    TODO();
} //	Replicate Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_MOVSQ(cs_x86 const& insn_detail) {
    TODO();
} //	Move Data From String to String
InterruptRaisedOr<void> CPU::handle_MOVSS(cs_x86 const& insn_detail) {
    TODO();
} //	Move or Merge Scalar Single Precision Floating-Point Value
InterruptRaisedOr<void> CPU::handle_MOVSW(cs_x86 const& insn_detail) {
    TODO();
} //	Move Data From String to String
InterruptRaisedOr<void> CPU::handle_MOVSX(cs_x86 const& insn_detail) {
    TODO();
} //	Move With Sign-Extension
InterruptRaisedOr<void> CPU::handle_MOVSXD(cs_x86 const& insn_detail) {
    TODO();
} //	Move With Sign-Extension
InterruptRaisedOr<void> CPU::handle_MOVUPD(cs_x86 const& insn_detail) {
    TODO();
} //	Move Unaligned Packed Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_MOVUPS(cs_x86 const& insn_detail) {
    TODO();
} //	Move Unaligned Packed Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_MOVZX(cs_x86 const& insn_detail) {
    TODO();
} //	Move With Zero-Extend
InterruptRaisedOr<void> CPU::handle_MPSADBW(cs_x86 const& insn_detail) {
    TODO();
} //	Compute Multiple Packed Sums of Absolute Difference
InterruptRaisedOr<void> CPU::handle_MUL(cs_x86 const& insn_detail) {
    TODO();
} //	Unsigned Multiply
InterruptRaisedOr<void> CPU::handle_MULPD(cs_x86 const& insn_detail) {
    TODO();
} //	Multiply Packed Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_MULPS(cs_x86 const& insn_detail) {
    TODO();
} //	Multiply Packed Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_MULSD(cs_x86 const& insn_detail) {
    TODO();
} //	Multiply Scalar Double Precision Floating-Point Value
InterruptRaisedOr<void> CPU::handle_MULSS(cs_x86 const& insn_detail) {
    TODO();
} //	Multiply Scalar Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_MULX(cs_x86 const& insn_detail) {
    TODO();
} //	Unsigned Multiply Without Affecting Flags
InterruptRaisedOr<void> CPU::handle_MWAIT(cs_x86 const& insn_detail) {
    TODO();
} //	Monitor Wait
InterruptRaisedOr<void> CPU::handle_NEG(cs_x86 const& insn_detail) {
    TODO();
} //	Two's Complement Negation
InterruptRaisedOr<void> CPU::handle_NOP(cs_x86 const& insn_detail) {
    TODO();
} //	No Operation
InterruptRaisedOr<void> CPU::handle_NOT(cs_x86 const& insn_detail) {
    TODO();
} //	One's Complement Negation
InterruptRaisedOr<void> CPU::handle_OR(cs_x86 const& insn_detail) {
    TODO();
} //	Logical Inclusive OR
InterruptRaisedOr<void> CPU::handle_ORPD(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical OR of Packed Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_ORPS(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical OR of Packed Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_OUT(cs_x86 const& insn_detail) {
    TODO();
} //	Output to Port
InterruptRaisedOr<void> CPU::handle_OUTS(cs_x86 const& insn_detail) {
    TODO();
} //	Output String to Port
InterruptRaisedOr<void> CPU::handle_OUTSB(cs_x86 const& insn_detail) {
    TODO();
} //	Output String to Port
InterruptRaisedOr<void> CPU::handle_OUTSD(cs_x86 const& insn_detail) {
    TODO();
} //	Output String to Port
InterruptRaisedOr<void> CPU::handle_OUTSW(cs_x86 const& insn_detail) {
    TODO();
} //	Output String to Port
InterruptRaisedOr<void> CPU::handle_PABSB(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Absolute Value
InterruptRaisedOr<void> CPU::handle_PABSD(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Absolute Value
InterruptRaisedOr<void> CPU::handle_PABSQ(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Absolute Value
InterruptRaisedOr<void> CPU::handle_PABSW(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Absolute Value
InterruptRaisedOr<void> CPU::handle_PACKSSDW(cs_x86 const& insn_detail) {
    TODO();
} //	Pack With Signed Saturation
InterruptRaisedOr<void> CPU::handle_PACKSSWB(cs_x86 const& insn_detail) {
    TODO();
} //	Pack With Signed Saturation
InterruptRaisedOr<void> CPU::handle_PACKUSDW(cs_x86 const& insn_detail) {
    TODO();
} //	Pack With Unsigned Saturation
InterruptRaisedOr<void> CPU::handle_PACKUSWB(cs_x86 const& insn_detail) {
    TODO();
} //	Pack With Unsigned Saturation
InterruptRaisedOr<void> CPU::handle_PADDB(cs_x86 const& insn_detail) {
    TODO();
} //	Add Packed Integers
InterruptRaisedOr<void> CPU::handle_PADDD(cs_x86 const& insn_detail) {
    TODO();
} //	Add Packed Integers
InterruptRaisedOr<void> CPU::handle_PADDQ(cs_x86 const& insn_detail) {
    TODO();
} //	Add Packed Integers
InterruptRaisedOr<void> CPU::handle_PADDSB(cs_x86 const& insn_detail) {
    TODO();
} //	Add Packed Signed Integers with Signed Saturation
InterruptRaisedOr<void> CPU::handle_PADDSW(cs_x86 const& insn_detail) {
    TODO();
} //	Add Packed Signed Integers with Signed Saturation
InterruptRaisedOr<void> CPU::handle_PADDUSB(cs_x86 const& insn_detail) {
    TODO();
} //	Add Packed Unsigned Integers With Unsigned Saturation
InterruptRaisedOr<void> CPU::handle_PADDUSW(cs_x86 const& insn_detail) {
    TODO();
} //	Add Packed Unsigned Integers With Unsigned Saturation
InterruptRaisedOr<void> CPU::handle_PADDW(cs_x86 const& insn_detail) {
    TODO();
} //	Add Packed Integers
InterruptRaisedOr<void> CPU::handle_PALIGNR(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Align Right
InterruptRaisedOr<void> CPU::handle_PAND(cs_x86 const& insn_detail) {
    TODO();
} //	Logical AND
InterruptRaisedOr<void> CPU::handle_PANDN(cs_x86 const& insn_detail) {
    TODO();
} //	Logical AND NOT
InterruptRaisedOr<void> CPU::handle_PAUSE(cs_x86 const& insn_detail) {
    TODO();
} //	Spin Loop Hint
InterruptRaisedOr<void> CPU::handle_PAVGB(cs_x86 const& insn_detail) {
    TODO();
} //	Average Packed Integers
InterruptRaisedOr<void> CPU::handle_PAVGW(cs_x86 const& insn_detail) {
    TODO();
} //	Average Packed Integers
InterruptRaisedOr<void> CPU::handle_PBLENDVB(cs_x86 const& insn_detail) {
    TODO();
} //	Variable Blend Packed Bytes
InterruptRaisedOr<void> CPU::handle_PBLENDW(cs_x86 const& insn_detail) {
    TODO();
} //	Blend Packed Words
InterruptRaisedOr<void> CPU::handle_PCLMULQDQ(cs_x86 const& insn_detail) {
    TODO();
} //	Carry-Less Multiplication Quadword
InterruptRaisedOr<void> CPU::handle_PCMPEQB(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Packed Data for Equal
InterruptRaisedOr<void> CPU::handle_PCMPEQD(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Packed Data for Equal
InterruptRaisedOr<void> CPU::handle_PCMPEQQ(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Packed Qword Data for Equal
InterruptRaisedOr<void> CPU::handle_PCMPEQW(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Packed Data for Equal
InterruptRaisedOr<void> CPU::handle_PCMPESTRI(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Compare Explicit Length Strings, Return Index
InterruptRaisedOr<void> CPU::handle_PCMPESTRM(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Compare Explicit Length Strings, Return Mask
InterruptRaisedOr<void> CPU::handle_PCMPGTB(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Packed Signed Integers for Greater Than
InterruptRaisedOr<void> CPU::handle_PCMPGTD(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Packed Signed Integers for Greater Than
InterruptRaisedOr<void> CPU::handle_PCMPGTQ(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Packed Data for Greater Than
InterruptRaisedOr<void> CPU::handle_PCMPGTW(cs_x86 const& insn_detail) {
    TODO();
} //	Compare Packed Signed Integers for Greater Than
InterruptRaisedOr<void> CPU::handle_PCMPISTRI(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Compare Implicit Length Strings, Return Index
InterruptRaisedOr<void> CPU::handle_PCMPISTRM(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Compare Implicit Length Strings, Return Mask
InterruptRaisedOr<void> CPU::handle_PCONFIG(cs_x86 const& insn_detail) {
    TODO();
} //	Platform Configuration
InterruptRaisedOr<void> CPU::handle_PDEP(cs_x86 const& insn_detail) {
    TODO();
} //	Parallel Bits Deposit
InterruptRaisedOr<void> CPU::handle_PEXT(cs_x86 const& insn_detail) {
    TODO();
} //	Parallel Bits Extract
InterruptRaisedOr<void> CPU::handle_PEXTRB(cs_x86 const& insn_detail) {
    TODO();
} //	Extract Byte/Dword/Qword
InterruptRaisedOr<void> CPU::handle_PEXTRD(cs_x86 const& insn_detail) {
    TODO();
} //	Extract Byte/Dword/Qword
InterruptRaisedOr<void> CPU::handle_PEXTRQ(cs_x86 const& insn_detail) {
    TODO();
} //	Extract Byte/Dword/Qword
InterruptRaisedOr<void> CPU::handle_PEXTRW(cs_x86 const& insn_detail) {
    TODO();
} //	Extract Word
InterruptRaisedOr<void> CPU::handle_PHADDD(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Horizontal Add
InterruptRaisedOr<void> CPU::handle_PHADDSW(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Horizontal Add and Saturate
InterruptRaisedOr<void> CPU::handle_PHADDW(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Horizontal Add
InterruptRaisedOr<void> CPU::handle_PHMINPOSUW(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Horizontal Word Minimum
InterruptRaisedOr<void> CPU::handle_PHSUBD(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Horizontal Subtract
InterruptRaisedOr<void> CPU::handle_PHSUBSW(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Horizontal Subtract and Saturate
InterruptRaisedOr<void> CPU::handle_PHSUBW(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Horizontal Subtract
InterruptRaisedOr<void> CPU::handle_PINSRB(cs_x86 const& insn_detail) {
    TODO();
} //	Insert Byte/Dword/Qword
InterruptRaisedOr<void> CPU::handle_PINSRD(cs_x86 const& insn_detail) {
    TODO();
} //	Insert Byte/Dword/Qword
InterruptRaisedOr<void> CPU::handle_PINSRQ(cs_x86 const& insn_detail) {
    TODO();
} //	Insert Byte/Dword/Qword
InterruptRaisedOr<void> CPU::handle_PINSRW(cs_x86 const& insn_detail) {
    TODO();
} //	Insert Word
InterruptRaisedOr<void> CPU::handle_PMADDUBSW(cs_x86 const& insn_detail) {
    TODO();
} //	Multiply and Add Packed Signed and Unsigned Bytes
InterruptRaisedOr<void> CPU::handle_PMADDWD(cs_x86 const& insn_detail) {
    TODO();
} //	Multiply and Add Packed Integers
InterruptRaisedOr<void> CPU::handle_PMAXSB(cs_x86 const& insn_detail) {
    TODO();
} //	Maximum of Packed Signed Integers
InterruptRaisedOr<void> CPU::handle_PMAXSD(cs_x86 const& insn_detail) {
    TODO();
} //	Maximum of Packed Signed Integers
InterruptRaisedOr<void> CPU::handle_PMAXSQ(cs_x86 const& insn_detail) {
    TODO();
} //	Maximum of Packed Signed Integers
InterruptRaisedOr<void> CPU::handle_PMAXSW(cs_x86 const& insn_detail) {
    TODO();
} //	Maximum of Packed Signed Integers
InterruptRaisedOr<void> CPU::handle_PMAXUB(cs_x86 const& insn_detail) {
    TODO();
} //	Maximum of Packed Unsigned Integers
InterruptRaisedOr<void> CPU::handle_PMAXUD(cs_x86 const& insn_detail) {
    TODO();
} //	Maximum of Packed Unsigned Integers
InterruptRaisedOr<void> CPU::handle_PMAXUQ(cs_x86 const& insn_detail) {
    TODO();
} //	Maximum of Packed Unsigned Integers
InterruptRaisedOr<void> CPU::handle_PMAXUW(cs_x86 const& insn_detail) {
    TODO();
} //	Maximum of Packed Unsigned Integers
InterruptRaisedOr<void> CPU::handle_PMINSB(cs_x86 const& insn_detail) {
    TODO();
} //	Minimum of Packed Signed Integers
InterruptRaisedOr<void> CPU::handle_PMINSD(cs_x86 const& insn_detail) {
    TODO();
} //	Minimum of Packed Signed Integers
InterruptRaisedOr<void> CPU::handle_PMINSQ(cs_x86 const& insn_detail) {
    TODO();
} //	Minimum of Packed Signed Integers
InterruptRaisedOr<void> CPU::handle_PMINSW(cs_x86 const& insn_detail) {
    TODO();
} //	Minimum of Packed Signed Integers
InterruptRaisedOr<void> CPU::handle_PMINUB(cs_x86 const& insn_detail) {
    TODO();
} //	Minimum of Packed Unsigned Integers
InterruptRaisedOr<void> CPU::handle_PMINUD(cs_x86 const& insn_detail) {
    TODO();
} //	Minimum of Packed Unsigned Integers
InterruptRaisedOr<void> CPU::handle_PMINUQ(cs_x86 const& insn_detail) {
    TODO();
} //	Minimum of Packed Unsigned Integers
InterruptRaisedOr<void> CPU::handle_PMINUW(cs_x86 const& insn_detail) {
    TODO();
} //	Minimum of Packed Unsigned Integers
InterruptRaisedOr<void> CPU::handle_PMOVMSKB(cs_x86 const& insn_detail) {
    TODO();
} //	Move Byte Mask
InterruptRaisedOr<void> CPU::handle_PMOVSX(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Move With Sign Extend
InterruptRaisedOr<void> CPU::handle_PMOVZX(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Move With Zero Extend
InterruptRaisedOr<void> CPU::handle_PMULDQ(cs_x86 const& insn_detail) {
    TODO();
} //	Multiply Packed Doubleword Integers
InterruptRaisedOr<void> CPU::handle_PMULHRSW(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Multiply High With Round and Scale
InterruptRaisedOr<void> CPU::handle_PMULHUW(cs_x86 const& insn_detail) {
    TODO();
} //	Multiply Packed Unsigned Integers and Store High Result
InterruptRaisedOr<void> CPU::handle_PMULHW(cs_x86 const& insn_detail) {
    TODO();
} //	Multiply Packed Signed Integers and Store High Result
InterruptRaisedOr<void> CPU::handle_PMULLD(cs_x86 const& insn_detail) {
    TODO();
} //	Multiply Packed Integers and Store Low Result
InterruptRaisedOr<void> CPU::handle_PMULLQ(cs_x86 const& insn_detail) {
    TODO();
} //	Multiply Packed Integers and Store Low Result
InterruptRaisedOr<void> CPU::handle_PMULLW(cs_x86 const& insn_detail) {
    TODO();
} //	Multiply Packed Signed Integers and Store Low Result
InterruptRaisedOr<void> CPU::handle_PMULUDQ(cs_x86 const& insn_detail) {
    TODO();
} //	Multiply Packed Unsigned Doubleword Integers
InterruptRaisedOr<void> CPU::handle_POP(cs_x86 const& insn_detail) {
    TODO();
} //	Pop a Value From the Stack
InterruptRaisedOr<void> CPU::handle_POPA(cs_x86 const& insn_detail) {
    TODO();
} //	Pop All General-Purpose Registers
InterruptRaisedOr<void> CPU::handle_POPAD(cs_x86 const& insn_detail) {
    TODO();
} //	Pop All General-Purpose Registers
InterruptRaisedOr<void> CPU::handle_POPCNT(cs_x86 const& insn_detail) {
    TODO();
} //	Return the Count of Number of Bits Set to 1
InterruptRaisedOr<void> CPU::handle_POPF(cs_x86 const& insn_detail) {
    TODO();
} //	Pop Stack Into EFLAGS Register
InterruptRaisedOr<void> CPU::handle_POPFD(cs_x86 const& insn_detail) {
    TODO();
} //	Pop Stack Into EFLAGS Register
InterruptRaisedOr<void> CPU::handle_POPFQ(cs_x86 const& insn_detail) {
    TODO();
} //	Pop Stack Into EFLAGS Register
InterruptRaisedOr<void> CPU::handle_POR(cs_x86 const& insn_detail) {
    TODO();
} //	Bitwise Logical OR
InterruptRaisedOr<void> CPU::handle_PREFETCHW(cs_x86 const& insn_detail) {
    TODO();
} //	Prefetch Data Into Caches in Anticipation of a Write
InterruptRaisedOr<void> CPU::handle_PREFETCHh(cs_x86 const& insn_detail) {
    TODO();
} //	Prefetch Data Into Caches
InterruptRaisedOr<void> CPU::handle_PSADBW(cs_x86 const& insn_detail) {
    TODO();
} //	Compute Sum of Absolute Differences
InterruptRaisedOr<void> CPU::handle_PSHUFB(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Shuffle Bytes
InterruptRaisedOr<void> CPU::handle_PSHUFD(cs_x86 const& insn_detail) {
    TODO();
} //	Shuffle Packed Doublewords
InterruptRaisedOr<void> CPU::handle_PSHUFHW(cs_x86 const& insn_detail) {
    TODO();
} //	Shuffle Packed High Words
InterruptRaisedOr<void> CPU::handle_PSHUFLW(cs_x86 const& insn_detail) {
    TODO();
} //	Shuffle Packed Low Words
InterruptRaisedOr<void> CPU::handle_PSHUFW(cs_x86 const& insn_detail) {
    TODO();
} //	Shuffle Packed Words
InterruptRaisedOr<void> CPU::handle_PSIGNB(cs_x86 const& insn_detail) {
    TODO();
} //	Packed SIGN
InterruptRaisedOr<void> CPU::handle_PSIGND(cs_x86 const& insn_detail) {
    TODO();
} //	Packed SIGN
InterruptRaisedOr<void> CPU::handle_PSIGNW(cs_x86 const& insn_detail) {
    TODO();
} //	Packed SIGN
InterruptRaisedOr<void> CPU::handle_PSLLD(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Packed Data Left Logical
InterruptRaisedOr<void> CPU::handle_PSLLDQ(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Double Quadword Left Logical
InterruptRaisedOr<void> CPU::handle_PSLLQ(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Packed Data Left Logical
InterruptRaisedOr<void> CPU::handle_PSLLW(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Packed Data Left Logical
InterruptRaisedOr<void> CPU::handle_PSRAD(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Packed Data Right Arithmetic
InterruptRaisedOr<void> CPU::handle_PSRAQ(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Packed Data Right Arithmetic
InterruptRaisedOr<void> CPU::handle_PSRAW(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Packed Data Right Arithmetic
InterruptRaisedOr<void> CPU::handle_PSRLD(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Packed Data Right Logical
InterruptRaisedOr<void> CPU::handle_PSRLDQ(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Double Quadword Right Logical
InterruptRaisedOr<void> CPU::handle_PSRLQ(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Packed Data Right Logical
InterruptRaisedOr<void> CPU::handle_PSRLW(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Packed Data Right Logical
InterruptRaisedOr<void> CPU::handle_PSUBB(cs_x86 const& insn_detail) {
    TODO();
} //	Subtract Packed Integers
InterruptRaisedOr<void> CPU::handle_PSUBD(cs_x86 const& insn_detail) {
    TODO();
} //	Subtract Packed Integers
InterruptRaisedOr<void> CPU::handle_PSUBQ(cs_x86 const& insn_detail) {
    TODO();
} //	Subtract Packed Quadword Integers
InterruptRaisedOr<void> CPU::handle_PSUBSB(cs_x86 const& insn_detail) {
    TODO();
} //	Subtract Packed Signed Integers With Signed Saturation
InterruptRaisedOr<void> CPU::handle_PSUBSW(cs_x86 const& insn_detail) {
    TODO();
} //	Subtract Packed Signed Integers With Signed Saturation
InterruptRaisedOr<void> CPU::handle_PSUBUSB(cs_x86 const& insn_detail) {
    TODO();
} //	Subtract Packed Unsigned Integers With Unsigned Saturation
InterruptRaisedOr<void> CPU::handle_PSUBUSW(cs_x86 const& insn_detail) {
    TODO();
} //	Subtract Packed Unsigned Integers With Unsigned Saturation
InterruptRaisedOr<void> CPU::handle_PSUBW(cs_x86 const& insn_detail) {
    TODO();
} //	Subtract Packed Integers
InterruptRaisedOr<void> CPU::handle_PTEST(cs_x86 const& insn_detail) {
    TODO();
} //	Logical Compare
InterruptRaisedOr<void> CPU::handle_PTWRITE(cs_x86 const& insn_detail) {
    TODO();
} //	Write Data to a Processor Trace Packet
InterruptRaisedOr<void> CPU::handle_PUNPCKHBW(cs_x86 const& insn_detail) {
    TODO();
} //	Unpack High Data
InterruptRaisedOr<void> CPU::handle_PUNPCKHDQ(cs_x86 const& insn_detail) {
    TODO();
} //	Unpack High Data
InterruptRaisedOr<void> CPU::handle_PUNPCKHQDQ(cs_x86 const& insn_detail) {
    TODO();
} //	Unpack High Data
InterruptRaisedOr<void> CPU::handle_PUNPCKHWD(cs_x86 const& insn_detail) {
    TODO();
} //	Unpack High Data
InterruptRaisedOr<void> CPU::handle_PUNPCKLBW(cs_x86 const& insn_detail) {
    TODO();
} //	Unpack Low Data
InterruptRaisedOr<void> CPU::handle_PUNPCKLDQ(cs_x86 const& insn_detail) {
    TODO();
} //	Unpack Low Data
InterruptRaisedOr<void> CPU::handle_PUNPCKLQDQ(cs_x86 const& insn_detail) {
    TODO();
} //	Unpack Low Data
InterruptRaisedOr<void> CPU::handle_PUNPCKLWD(cs_x86 const& insn_detail) {
    TODO();
} //	Unpack Low Data
InterruptRaisedOr<void> CPU::handle_PUSH(cs_x86 const& insn_detail) {
    TODO();
} //	Push Word, Doubleword, or Quadword Onto the Stack
InterruptRaisedOr<void> CPU::handle_PUSHA(cs_x86 const& insn_detail) {
    TODO();
} //	Push All General-Purpose Registers
InterruptRaisedOr<void> CPU::handle_PUSHAD(cs_x86 const& insn_detail) {
    TODO();
} //	Push All General-Purpose Registers
InterruptRaisedOr<void> CPU::handle_PUSHF(cs_x86 const& insn_detail) {
    TODO();
} //	Push EFLAGS Register Onto the Stack
InterruptRaisedOr<void> CPU::handle_PUSHFD(cs_x86 const& insn_detail) {
    TODO();
} //	Push EFLAGS Register Onto the Stack
InterruptRaisedOr<void> CPU::handle_PUSHFQ(cs_x86 const& insn_detail) {
    TODO();
} //	Push EFLAGS Register Onto the Stack
InterruptRaisedOr<void> CPU::handle_PXOR(cs_x86 const& insn_detail) {
    TODO();
} //	Logical Exclusive OR
InterruptRaisedOr<void> CPU::handle_RCL(cs_x86 const& insn_detail) {
    TODO();
} //	Rotate
InterruptRaisedOr<void> CPU::handle_RCPPS(cs_x86 const& insn_detail) {
    TODO();
} //	Compute Reciprocals of Packed Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_RCPSS(cs_x86 const& insn_detail) {
    TODO();
} //	Compute Reciprocal of Scalar Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_RCR(cs_x86 const& insn_detail) {
    TODO();
} //	Rotate
InterruptRaisedOr<void> CPU::handle_RDFSBASE(cs_x86 const& insn_detail) {
    TODO();
} //	Read FS/GS Segment Base
InterruptRaisedOr<void> CPU::handle_RDGSBASE(cs_x86 const& insn_detail) {
    TODO();
} //	Read FS/GS Segment Base
InterruptRaisedOr<void> CPU::handle_RDMSR(cs_x86 const& insn_detail) {
    TODO();
} //	Read From Model Specific Register
InterruptRaisedOr<void> CPU::handle_RDPID(cs_x86 const& insn_detail) {
    TODO();
} //	Read Processor ID
InterruptRaisedOr<void> CPU::handle_RDPKRU(cs_x86 const& insn_detail) {
    TODO();
} //	Read Protection Key Rights for User Pages
InterruptRaisedOr<void> CPU::handle_RDPMC(cs_x86 const& insn_detail) {
    TODO();
} //	Read Performance-Monitoring Counters
InterruptRaisedOr<void> CPU::handle_RDRAND(cs_x86 const& insn_detail) {
    TODO();
} //	Read Random Number
InterruptRaisedOr<void> CPU::handle_RDSEED(cs_x86 const& insn_detail) {
    TODO();
} //	Read Random SEED
InterruptRaisedOr<void> CPU::handle_RDSSPD(cs_x86 const& insn_detail) {
    TODO();
} //	Read Shadow Stack Pointer
InterruptRaisedOr<void> CPU::handle_RDSSPQ(cs_x86 const& insn_detail) {
    TODO();
} //	Read Shadow Stack Pointer
InterruptRaisedOr<void> CPU::handle_RDTSC(cs_x86 const& insn_detail) {
    TODO();
} //	Read Time-Stamp Counter
InterruptRaisedOr<void> CPU::handle_RDTSCP(cs_x86 const& insn_detail) {
    TODO();
} //	Read Time-Stamp Counter and Processor ID
InterruptRaisedOr<void> CPU::handle_REP(cs_x86 const& insn_detail) {
    TODO();
} //	Repeat String Operation Prefix
InterruptRaisedOr<void> CPU::handle_REPE(cs_x86 const& insn_detail) {
    TODO();
} //	Repeat String Operation Prefix
InterruptRaisedOr<void> CPU::handle_REPNE(cs_x86 const& insn_detail) {
    TODO();
} //	Repeat String Operation Prefix
InterruptRaisedOr<void> CPU::handle_REPNZ(cs_x86 const& insn_detail) {
    TODO();
} //	Repeat String Operation Prefix
InterruptRaisedOr<void> CPU::handle_REPZ(cs_x86 const& insn_detail) {
    TODO();
} //	Repeat String Operation Prefix
InterruptRaisedOr<void> CPU::handle_RET(cs_x86 const& insn_detail) {
    TODO();
} //	Return From Procedure
InterruptRaisedOr<void> CPU::handle_ROL(cs_x86 const& insn_detail) {
    TODO();
} //	Rotate
InterruptRaisedOr<void> CPU::handle_ROR(cs_x86 const& insn_detail) {
    TODO();
} //	Rotate
InterruptRaisedOr<void> CPU::handle_RORX(cs_x86 const& insn_detail) {
    TODO();
} //	Rotate Right Logical Without Affecting Flags
InterruptRaisedOr<void> CPU::handle_ROUNDPD(cs_x86 const& insn_detail) {
    TODO();
} //	Round Packed Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_ROUNDPS(cs_x86 const& insn_detail) {
    TODO();
} //	Round Packed Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_ROUNDSD(cs_x86 const& insn_detail) {
    TODO();
} //	Round Scalar Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_ROUNDSS(cs_x86 const& insn_detail) {
    TODO();
} //	Round Scalar Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_RSM(cs_x86 const& insn_detail) {
    TODO();
} //	Resume From System Management Mode
InterruptRaisedOr<void> CPU::handle_RSQRTPS(cs_x86 const& insn_detail) {
    TODO();
} //	Compute Reciprocals of Square Roots of Packed Single Precision Floating-PointValues
InterruptRaisedOr<void> CPU::handle_RSQRTSS(cs_x86 const& insn_detail) {
    TODO();
} //	Compute Reciprocal of Square Root of Scalar Single Precision Floating-Point Value
InterruptRaisedOr<void> CPU::handle_RSTORSSP(cs_x86 const& insn_detail) {
    TODO();
} //	Restore Saved Shadow Stack Pointer
InterruptRaisedOr<void> CPU::handle_SAHF(cs_x86 const& insn_detail) {
    TODO();
} //	Store AH Into Flags
InterruptRaisedOr<void> CPU::handle_SAL(cs_x86 const& insn_detail) {
    TODO();
} //	Shift
InterruptRaisedOr<void> CPU::handle_SAR(cs_x86 const& insn_detail) {
    TODO();
} //	Shift
InterruptRaisedOr<void> CPU::handle_SARX(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Without Affecting Flags
InterruptRaisedOr<void> CPU::handle_SAVEPREVSSP(cs_x86 const& insn_detail) {
    TODO();
} //	Save Previous Shadow Stack Pointer
InterruptRaisedOr<void> CPU::handle_SBB(cs_x86 const& insn_detail) {
    TODO();
} //	Integer Subtraction With Borrow
InterruptRaisedOr<void> CPU::handle_SCAS(cs_x86 const& insn_detail) {
    TODO();
} //	Scan String
InterruptRaisedOr<void> CPU::handle_SCASB(cs_x86 const& insn_detail) {
    TODO();
} //	Scan String
InterruptRaisedOr<void> CPU::handle_SCASD(cs_x86 const& insn_detail) {
    TODO();
} //	Scan String
InterruptRaisedOr<void> CPU::handle_SCASW(cs_x86 const& insn_detail) {
    TODO();
} //	Scan String
InterruptRaisedOr<void> CPU::handle_SENDUIPI(cs_x86 const& insn_detail) {
    TODO();
} //	Send User Interprocessor Interrupt
InterruptRaisedOr<void> CPU::handle_SERIALIZE(cs_x86 const& insn_detail) {
    TODO();
} //	Serialize Instruction Execution
InterruptRaisedOr<void> CPU::handle_SETSSBSY(cs_x86 const& insn_detail) {
    TODO();
} //	Mark Shadow Stack Busy
InterruptRaisedOr<void> CPU::handle_SETcc(cs_x86 const& insn_detail) {
    TODO();
} //	Set Byte on Condition
InterruptRaisedOr<void> CPU::handle_SFENCE(cs_x86 const& insn_detail) {
    TODO();
} //	Store Fence
InterruptRaisedOr<void> CPU::handle_SGDT(cs_x86 const& insn_detail) {
    TODO();
} //	Store Global Descriptor Table Register
InterruptRaisedOr<void> CPU::handle_SHA1MSG1(cs_x86 const& insn_detail) {
    TODO();
} //	Perform an Intermediate Calculation for the Next Four SHA1 Message Dwords
InterruptRaisedOr<void> CPU::handle_SHA1MSG2(cs_x86 const& insn_detail) {
    TODO();
} //	Perform a Final Calculation for the Next Four SHA1 Message Dwords
InterruptRaisedOr<void> CPU::handle_SHA1NEXTE(cs_x86 const& insn_detail) {
    TODO();
} //	Calculate SHA1 State Variable E After Four Rounds
InterruptRaisedOr<void> CPU::handle_SHA1RNDS4(cs_x86 const& insn_detail) {
    TODO();
} //	Perform Four Rounds of SHA1 Operation
InterruptRaisedOr<void> CPU::handle_SHA256MSG1(cs_x86 const& insn_detail) {
    TODO();
} //	Perform an Intermediate Calculation for the Next Four SHA256 MessageDwords
InterruptRaisedOr<void> CPU::handle_SHA256MSG2(cs_x86 const& insn_detail) {
    TODO();
} //	Perform a Final Calculation for the Next Four SHA256 Message Dwords
InterruptRaisedOr<void> CPU::handle_SHA256RNDS2(cs_x86 const& insn_detail) {
    TODO();
} //	Perform Two Rounds of SHA256 Operation
InterruptRaisedOr<void> CPU::handle_SHL(cs_x86 const& insn_detail) {
    TODO();
} //	Shift
InterruptRaisedOr<void> CPU::handle_SHLD(cs_x86 const& insn_detail) {
    TODO();
} //	Double Precision Shift Left
InterruptRaisedOr<void> CPU::handle_SHLX(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Without Affecting Flags
InterruptRaisedOr<void> CPU::handle_SHR(cs_x86 const& insn_detail) {
    TODO();
} //	Shift
InterruptRaisedOr<void> CPU::handle_SHRD(cs_x86 const& insn_detail) {
    TODO();
} //	Double Precision Shift Right
InterruptRaisedOr<void> CPU::handle_SHRX(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Without Affecting Flags
InterruptRaisedOr<void> CPU::handle_SHUFPD(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Interleave Shuffle of Pairs of Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_SHUFPS(cs_x86 const& insn_detail) {
    TODO();
} //	Packed Interleave Shuffle of Quadruplets of Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_SIDT(cs_x86 const& insn_detail) {
    TODO();
} //	Store Interrupt Descriptor Table Register
InterruptRaisedOr<void> CPU::handle_SLDT(cs_x86 const& insn_detail) {
    TODO();
} //	Store Local Descriptor Table Register
InterruptRaisedOr<void> CPU::handle_SMSW(cs_x86 const& insn_detail) {
    TODO();
} //	Store Machine Status Word
InterruptRaisedOr<void> CPU::handle_SQRTPD(cs_x86 const& insn_detail) {
    TODO();
} //	Square Root of Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_SQRTPS(cs_x86 const& insn_detail) {
    TODO();
} //	Square Root of Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_SQRTSD(cs_x86 const& insn_detail) {
    TODO();
} //	Compute Square Root of Scalar Double Precision Floating-Point Value
InterruptRaisedOr<void> CPU::handle_SQRTSS(cs_x86 const& insn_detail) {
    TODO();
} //	Compute Square Root of Scalar Single Precision Value
InterruptRaisedOr<void> CPU::handle_STAC(cs_x86 const& insn_detail) {
    TODO();
} //	Set AC Flag in EFLAGS Register
InterruptRaisedOr<void> CPU::handle_STC(cs_x86 const& insn_detail) {
    TODO();
} //	Set Carry Flag
InterruptRaisedOr<void> CPU::handle_STD(cs_x86 const& insn_detail) {
    TODO();
} //	Set Direction Flag
InterruptRaisedOr<void> CPU::handle_STI(cs_x86 const& insn_detail) {
    TODO();
} //	Set Interrupt Flag
InterruptRaisedOr<void> CPU::handle_STMXCSR(cs_x86 const& insn_detail) {
    TODO();
} //	Store MXCSR Register State
InterruptRaisedOr<void> CPU::handle_STOS(cs_x86 const& insn_detail) {
    TODO();
} //	Store String
InterruptRaisedOr<void> CPU::handle_STOSB(cs_x86 const& insn_detail) {
    TODO();
} //	Store String
InterruptRaisedOr<void> CPU::handle_STOSD(cs_x86 const& insn_detail) {
    TODO();
} //	Store String
InterruptRaisedOr<void> CPU::handle_STOSQ(cs_x86 const& insn_detail) {
    TODO();
} //	Store String
InterruptRaisedOr<void> CPU::handle_STOSW(cs_x86 const& insn_detail) {
    TODO();
} //	Store String
InterruptRaisedOr<void> CPU::handle_STR(cs_x86 const& insn_detail) {
    TODO();
} //	Store Task Register
InterruptRaisedOr<void> CPU::handle_STTILECFG(cs_x86 const& insn_detail) {
    TODO();
} //	Store Tile Configuration
InterruptRaisedOr<void> CPU::handle_STUI(cs_x86 const& insn_detail) {
    TODO();
} //	Set User Interrupt Flag
InterruptRaisedOr<void> CPU::handle_SUB(cs_x86 const& insn_detail) {
    TODO();
} //	Subtract
InterruptRaisedOr<void> CPU::handle_SUBPD(cs_x86 const& insn_detail) {
    TODO();
} //	Subtract Packed Double Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_SUBPS(cs_x86 const& insn_detail) {
    TODO();
} //	Subtract Packed Single Precision Floating-Point Values
InterruptRaisedOr<void> CPU::handle_SUBSD(cs_x86 const& insn_detail) {
    TODO();
} //	Subtract Scalar Double Precision Floating-Point Value
InterruptRaisedOr<void> CPU::handle_SUBSS(cs_x86 const& insn_detail) {
    TODO();
} //	Subtract Scalar Single Precision Floating-Point Value
InterruptRaisedOr<void> CPU::handle_SWAPGS(cs_x86 const& insn_detail) {
    TODO();
} //	Swap GS Base Register
InterruptRaisedOr<void> CPU::handle_SYSCALL(cs_x86 const& insn_detail) {
    TODO();
} //	Fast System Call
InterruptRaisedOr<void> CPU::handle_SYSENTER(cs_x86 const& insn_detail) {
    TODO();
} //	Fast System Call
InterruptRaisedOr<void> CPU::handle_SYSEXIT(cs_x86 const& insn_detail) {
    TODO();
} //	Fast Return from Fast System Call
InterruptRaisedOr<void> CPU::handle_SYSRET(cs_x86 const& insn_detail) {
    TODO();
} //	Return From Fast System Call
InterruptRaisedOr<void> CPU::handle_TDPBF16PS(cs_x86 const& insn_detail) {
    TODO();
} //	Dot Product of BF16 Tiles Accumulated into Packed Single Precision Tile
InterruptRaisedOr<void> CPU::handle_TDPBSSD(cs_x86 const& insn_detail) {
    TODO();
} //	Dot Product of Signed/Unsigned Bytes with DwordAccumulation
InterruptRaisedOr<void> CPU::handle_TDPBSUD(cs_x86 const& insn_detail) {
    TODO();
} //	Dot Product of Signed/Unsigned Bytes with DwordAccumulation
InterruptRaisedOr<void> CPU::handle_TDPBUSD(cs_x86 const& insn_detail) {
    TODO();
} //	Dot Product of Signed/Unsigned Bytes with DwordAccumulation
InterruptRaisedOr<void> CPU::handle_TDPBUUD(cs_x86 const& insn_detail) {
    TODO();
} //	Dot Product of Signed/Unsigned Bytes with DwordAccumulation
InterruptRaisedOr<void> CPU::handle_TEST(cs_x86 const& insn_detail) {
    TODO();
} //	Logical Compare
InterruptRaisedOr<void> CPU::handle_TESTUI(cs_x86 const& insn_detail) {
    TODO();
} //	Determine User Interrupt Flag
InterruptRaisedOr<void> CPU::handle_TILELOADD(cs_x86 const& insn_detail) {
    TODO();
} //	Load Tile
InterruptRaisedOr<void> CPU::handle_TILERELEASE(cs_x86 const& insn_detail) {
    TODO();
} //	Release Tile
InterruptRaisedOr<void> CPU::handle_TILESTORED(cs_x86 const& insn_detail) {
    TODO();
} //	Store Tile
InterruptRaisedOr<void> CPU::handle_TILEZERO(cs_x86 const& insn_detail) {
    TODO();
} //	Zero Tile
InterruptRaisedOr<void> CPU::handle_TPAUSE(cs_x86 const& insn_detail) {
    TODO();
} //	Timed PAUSE
InterruptRaisedOr<void> CPU::handle_TZCNT(cs_x86 const& insn_detail) {
    TODO();
} //	Count the Number of Trailing Zero Bits
InterruptRaisedOr<void> CPU::handle_UCOMISD(cs_x86 const& insn_detail) {
    TODO();
} //	Unordered Compare Scalar Double Precision Floating-Point Values and Set EFLAGS
InterruptRaisedOr<void> CPU::handle_UCOMISS(cs_x86 const& insn_detail) {
    TODO();
} //	Unordered Compare Scalar Single Precision Floating-Point Values and Set EFLAGS
InterruptRaisedOr<void> CPU::handle_UD(cs_x86 const& insn_detail) {
    TODO();
} //	Undefined Instruction
InterruptRaisedOr<void> CPU::handle_UIRET(cs_x86 const& insn_detail) {
    TODO();
} //


}
