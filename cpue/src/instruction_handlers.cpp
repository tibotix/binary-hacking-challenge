
#include "cpu.h"

namespace CPUE {

// TODO: use dispatch array, and build it in constexpr, use nullptr for not implemented and pointer to member function otherwise
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_insn(cs_insn const& insn) {
    CPUE_TRACE("Handling instruction: {} {}", insn.mnemonic, insn.op_str);
    auto const& detail = insn.detail->x86;
#define CASE(name) \
    case x86_insn::X86_INS_##name: return handle_##name(detail); break;

    switch (insn.id) {
        CASE(ADD)
        CASE(BOUND)
        CASE(CLI)
        CASE(CMP)
        CASE(DEC)
        CASE(DIV)
        CASE(HLT)
        CASE(IDIV)
        CASE(IMUL)
        CASE(INC)
        CASE(INT)
        CASE(INT1)
        CASE(INT3)
        CASE(INTO)
        CASE(INVLPG)
        CASE(IRET)
        CASE(IRETD)
        CASE(IRETQ)
        CASE(JMP)
        CASE(JNE)
        CASE(JE)
        CASE(JGE)
        CASE(JG)
        CASE(JLE)
        CASE(JL)
        CASE(LEA)
        CASE(LEAVE)
        CASE(LGDT)
        CASE(LIDT)
        CASE(LLDT)
        CASE(LTR)
        CASE(LOOP)
        CASE(MOV)
        CASE(MOVABS)
        CASE(MOVSX)
        CASE(MOVSXD)
        CASE(MOVZX)
        CASE(MUL)
        CASE(NOP)
        CASE(NOT)
        CASE(OR)
        CASE(POP)
        CASE(POPF)
        CASE(POPFQ)
        CASE(PUSH)
        CASE(PUSHF)
        CASE(PUSHFQ)
        CASE(RET)
        CASE(ROL)
        CASE(ROR)
        CASE(SAL)
        CASE(SAR)
        CASE(SGDT)
        CASE(SHL)
        CASE(SHLD)
        CASE(SHLX)
        CASE(SHR)
        CASE(SIDT)
        CASE(SLDT)
        CASE(STI)
        CASE(SUB)
        CASE(SWAPGS)
        CASE(TEST)
        CASE(XCHG)
        CASE(XOR)

        default: TODO("Unhandled instruction.");
    }
#undef CASE
}


/**
 * Helper Functions:
 */

template<unsigned_integral R, unsigned_integral T>
requires(sizeof(R) >= sizeof(T)) constexpr R zero_extend(T value) {
    return value;
}
template<unsigned_integral R, unsigned_integral T>
requires(sizeof(R) >= sizeof(T)) constexpr R sign_extend(T value) {
    if (sizeof(T) * 8 >= 64)
        return value;
    if (sign_bit(value))
        return (static_cast<R>(-1) << sizeof(T) * 8) | value;
    return value;
}
static SizedValue sign_extend(SizedValue const& value, ByteWidth width) {
    if (value.byte_width() >= ByteWidth::WIDTH_QWORD)
        return value;
    if (value.sign_bit())
        return {(width.bitmask() << value.bit_width()) | value.value(), width};
    return value;
}

InterruptRaisedOr<void> CPU::do_privileged_instruction_check(u8 pl) {
    if (cpl() != pl)
        return raise_integral_interrupt(Exceptions::GP(ZERO_ERROR_CODE_NOEXT));
    return {};
}


/**
 * Handler Implementations:
 */

InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_ADD(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto second_val = MAY_HAVE_RAISED(second_op.read());
    if (second_op.operand().type == X86_OP_IMM)
        second_val = sign_extend(second_val, first_val.byte_width());
    auto res = CPUE_checked_single_uadd(first_val, second_val);

    update_rflags(res);
    MAY_HAVE_RAISED(first_op.write(res.value));

    return INCREMENT_IP;
} //	Add
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_BOUND(cs_x86 const& insn_detail) {
    // TODO: probably don't implement this insn
    TODO("Only handle interrupt if out of bounds");
    static Interrupt i = {
        .vector = 5,
        .type = InterruptType::SOFTWARE_INTERRUPT,
        .iclass = InterruptClass::BENIGN,
    };
    MAY_HAVE_RAISED(handle_interrupt(i));

    return INCREMENT_IP;
} //	Check Array Index Against Bounds
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_CLI(cs_x86 const& insn_detail) {
    if (cr0().c.PE) {
        m_rflags.c.IF = 0;
    } else if (m_rflags.c.IOPL >= cpl()) {
        m_rflags.c.IF = 0;
    } else if (/*VME Mode*/ (m_rflags.c.VM && cr4().c.VME) || /*PVI Mode*/ (!m_rflags.c.VM && cpl() == 3 && cr4().c.PVI)) {
        m_rflags.c.VIF = 0;
    } else {
        return raise_integral_interrupt(Exceptions::GP(ZERO_ERROR_CODE_NOEXT));
    }
    return INCREMENT_IP;
} //    Clear Interrupt Flag
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_CMP(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto second_val = MAY_HAVE_RAISED(second_op.read());
    if (second_op.operand().type == X86_OP_IMM)
        second_val = sign_extend(second_val, first_val.byte_width());
    auto res = CPUE_checked_single_usub(first_val, second_val);

    update_rflags(res);

    return INCREMENT_IP;
} //    Compare Two Operands
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_DEC(cs_x86 const& insn_detail) {
    TODO();
} //    Decrement By 1
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_DIV(cs_x86 const& insn_detail) {
    TODO();
} //	Unsigned Divide
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_HLT(cs_x86 const& insn_detail) {
    CPUE_INFO("encountered a HLT instruction. We use this instruction to exit the emulator (although normally it would behave quite differently)");
    shutdown();
} //	Halt
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_IDIV(cs_x86 const& insn_detail) {
    TODO();
} //	Signed Divide
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_IMUL(cs_x86 const& insn_detail) {
    TODO();
} //	Signed Multiply
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_INC(cs_x86 const& insn_detail) {
    TODO();
} //	Increment by 1
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_INT(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    Interrupt i = {
        .vector = MAY_HAVE_RAISED(first_op.read()).as<InterruptVector>(),
        .type = InterruptType::SOFTWARE_INTERRUPT,
        .iclass = InterruptClass::BENIGN,
        .source = InterruptSource::INTN_INT3_INTO_INSN,
    };

    // update rip to next instruction, because return address is next insn, so that handle_interrupt can use it
    m_rip_val = next_insn_rip();
    MAY_HAVE_RAISED(handle_interrupt(i));
    return DONT_INCREMENT_IP;
} // 	Call to Interrupt Procedure
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_INT1(cs_x86 const& insn_detail) {
    static Interrupt i = {
        .vector = 1,
        .type = InterruptType::SOFTWARE_INTERRUPT,
        .iclass = InterruptClass::BENIGN,
        .source = InterruptSource::INT1_INSN,
    };
    // update rip to next instruction, because return address is next insn, so that handle_interrupt can use it
    m_rip_val = next_insn_rip();
    MAY_HAVE_RAISED(handle_interrupt(i));
    return DONT_INCREMENT_IP;
} //	Call to Interrupt Procedure
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_INT3(cs_x86 const& insn_detail) {
    static Interrupt i = {
        .vector = 3,
        .type = InterruptType::SOFTWARE_INTERRUPT,
        .iclass = InterruptClass::BENIGN,
        .source = InterruptSource::INTN_INT3_INTO_INSN,
    };
    // update rip to next instruction, because return address is next insn, so that handle_interrupt can use it
    m_rip_val = next_insn_rip();
    MAY_HAVE_RAISED(handle_interrupt(i));
    return DONT_INCREMENT_IP;
} //	Call to Interrupt Procedure
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_INTO(cs_x86 const& insn_detail) {
    static Interrupt i = {
        .vector = 4,
        .type = InterruptType::SOFTWARE_INTERRUPT,
        .iclass = InterruptClass::BENIGN,
        .source = InterruptSource::INTN_INT3_INTO_INSN,
    };
    // update rip to next instruction, because return address is next insn, so that handle_interrupt can use it
    m_rip_val = next_insn_rip();
    if (m_rflags.c.OF) {
        MAY_HAVE_RAISED(handle_interrupt(i));
    }
    return DONT_INCREMENT_IP;
} //	Call to Interrupt Procedure
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_INVLPG(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(do_privileged_instruction_check());
    TODO();
} //	Invalidate TLB Entries
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_IRET(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(do_privileged_instruction_check());
    TODO();
} //	Interrupt Return
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_IRETD(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(do_privileged_instruction_check());
    TODO();
} //	Interrupt Return
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_IRETQ(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(do_privileged_instruction_check());
    TODO();
} //	Interrupt Return
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_JMP(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);

    TODO_NOFAIL("Check for far jumps.");
    m_rip_val = MAY_HAVE_RAISED(first_op.read()).value();
    return DONT_INCREMENT_IP;
} // Jump
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_JNE(cs_x86 const& insn_detail) {
    if (!m_rflags.c.ZF)
        return handle_JMP(insn_detail);
    return INCREMENT_IP;
} //	Jump Not Equal
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_JE(cs_x86 const& insn_detail) {
    if (m_rflags.c.ZF)
        return handle_JMP(insn_detail);
    return INCREMENT_IP;
} //	Jump Equal
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_JGE(cs_x86 const& insn_detail) {
    if (m_rflags.c.SF == m_rflags.c.OF)
        return handle_JMP(insn_detail);
    return INCREMENT_IP;
} //	Jump Greater or Equal
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_JG(cs_x86 const& insn_detail) {
    if (!m_rflags.c.ZF && m_rflags.c.SF == m_rflags.c.OF)
        return handle_JMP(insn_detail);
    return INCREMENT_IP;
} //	Jump Greater
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_JLE(cs_x86 const& insn_detail) {
    if (m_rflags.c.ZF && m_rflags.c.SF != m_rflags.c.OF)
        return handle_JMP(insn_detail);
    return INCREMENT_IP;
} //	Jump Lower or Equal
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_JL(cs_x86 const& insn_detail) {
    if (m_rflags.c.SF != m_rflags.c.OF)
        return handle_JMP(insn_detail);
    return INCREMENT_IP;
} //	Jump Lower
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_LEA(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    if (second_op.operand().type != X86_OP_MEM)
        return raise_integral_interrupt(Exceptions::UD());
    auto offset = MAY_HAVE_RAISED(operand_mem_offset(second_op.operand().mem));
    offset &= bytemask(insn_detail.addr_size);

    MAY_HAVE_RAISED(first_op.write(SizedValue(offset, first_op.byte_width())));

    return INCREMENT_IP;
} //	Load Effective Address
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_LEAVE(cs_x86 const& insn_detail) {
    TODO();
} //	High Level Procedure Exit
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_LGDT(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(do_privileged_instruction_check());
    TODO();
} //	Load Global/Interrupt Descriptor Table Register
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_LIDT(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(do_privileged_instruction_check());
    TODO();
} //	Load Global/Interrupt Descriptor Table Register
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_LLDT(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(do_privileged_instruction_check());
    TODO();
} //	Load Local Descriptor Table Register
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_LTR(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(do_privileged_instruction_check());
    TODO();
} //	Load Task Register
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_LOOP(cs_x86 const& insn_detail) {
    // TODO: no high priority
    TODO();
} //	Loop According to ECX Counter
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_MOV(cs_x86 const& insn_detail) {
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
    auto first_op = Operand(this, insn_detail.operands[0]); // Destination
    auto second_op = Operand(this, insn_detail.operands[1]); // Source

    // Prevent the MOV instruction from attempting to load the CS register
    if (first_op.operand().type == X86_OP_REG && first_op.operand().reg == X86_REG_CS) {
        fail("Cannot load CS register using the MOV instruction. Use far JMP, CALL, or RET instead.");
    }

    // Read the value from the source operand
    auto second_val = MAY_HAVE_RAISED(second_op.read());

    if (second_op.operand().type == X86_OP_IMM)
        second_val = sign_extend(second_val, first_op.byte_width());

    MAY_HAVE_RAISED(first_op.write(second_val));

    return INCREMENT_IP;
} //	Move
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_MOVABS(cs_x86 const& insn_detail) {
    return handle_MOV(insn_detail);
} //	Move With Sign-Extension
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_MOVSX(cs_x86 const& insn_detail) {
    TODO();
} //	Move With Sign-Extension
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_MOVSXD(cs_x86 const& insn_detail) {
    TODO();
} //	Move With Sign-Extension
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_MOVZX(cs_x86 const& insn_detail) {
    TODO();
} //	Move With Zero-Extend
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_MUL(cs_x86 const& insn_detail) {
    // TODO: fix this
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto second_val = MAY_HAVE_RAISED(second_op.read());
    if (second_op.operand().type == X86_OP_IMM)
        second_val = sign_extend(second_val, first_val.byte_width());
    auto res = CPUE_checked_single_umul(first_val, second_val);

    TODO("handle_MUL split registers to make 128bits");

    update_rflags(res);
    MAY_HAVE_RAISED(first_op.write(res.value));

    return INCREMENT_IP;
} //	Unsigned Multiply
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_NOP(cs_x86 const& insn_detail) {
    return INCREMENT_IP;
} //	No Operation
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_NOT(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);

    auto value = MAY_HAVE_RAISED(first_op.read());
    auto not_value = ~value;

    MAY_HAVE_RAISED(first_op.write(not_value));

    return INCREMENT_IP;
} //	One's Complement Negation
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_OR(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto second_val = sign_extend(MAY_HAVE_RAISED(second_op.read()), first_val.byte_width());

    auto ored_value = first_val | second_val;

    // The OF and CF flags are cleared; the SF, ZF, and PF flags are set according to the result. The state of the AF flag is undefined.
    m_rflags.c.OF = m_rflags.c.CF = 0;
    update_rflags(ored_value);

    return INCREMENT_IP;
} //	Logical Inclusive OR
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_POP(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);

    auto value = MAY_HAVE_RAISED(stack_pop(INTENTION_HANDLE_INSTRUCTION));

    MAY_HAVE_RAISED(first_op.write(SizedValue(value)));

    return INCREMENT_IP;
} //	Pop a Value From the Stack
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_POPF(cs_x86 const& insn_detail) {
    TODO();
} //	Pop Stack Into lower 16 bits of RFLAGS Register
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_POPFQ(cs_x86 const& insn_detail) {
    TODO();
} //	Pop Stack Into RFLAGS Register
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_PUSH(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);

    auto value = MAY_HAVE_RAISED(first_op.read());

    MAY_HAVE_RAISED(stack_push(value.value()));
    return INCREMENT_IP;
} //	Push Word, Doubleword, or Quadword Onto the Stack
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_PUSHF(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(stack_push(m_rflags.value & 0x000000000000FFFF));
    return INCREMENT_IP;
} //	Push lower 16 bits of RFLAGS Register Onto the Stack
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_PUSHFQ(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(stack_push(m_rflags.value & 0x0000000000FCFFFF));
    return INCREMENT_IP;
} //	Push RFLAGS Register Onto the Stack
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_RET(cs_x86 const& insn_detail) {
    TODO();
} //	Return From Procedure
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_ROL(cs_x86 const& insn_detail) {
    TODO();
} //	Rotate
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_ROR(cs_x86 const& insn_detail) {
    TODO();
} //	Rotate
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_SAL(cs_x86 const& insn_detail) {
    TODO();
} //	Shift
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_SAR(cs_x86 const& insn_detail) {
    TODO();
} //	Shift
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_SGDT(cs_x86 const& insn_detail) {
    TODO();
} //	Store Global Descriptor Table Register
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_SHL(cs_x86 const& insn_detail) {
    TODO();
} //	Shift
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_SHLD(cs_x86 const& insn_detail) {
    TODO();
} //	Double Precision Shift Left
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_SHLX(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Without Affecting Flags
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_SHR(cs_x86 const& insn_detail) {
    TODO();
} //	Shift
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_SIDT(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(do_privileged_instruction_check());
    TODO();
} //	Store Interrupt Descriptor Table Register
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_SLDT(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(do_privileged_instruction_check());
    TODO();
} //	Store Local Descriptor Table Register
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_STI(cs_x86 const& insn_detail) {
    if (cr0().c.PE) {
        m_rflags.c.IF = 1;
    } else if (m_rflags.c.IOPL >= cpl()) {
        m_rflags.c.IF = 1;
    } else if ((/*VME Mode*/ (m_rflags.c.VM && cr4().c.VME) || /*PVI Mode*/ (!m_rflags.c.VM && cpl() == 3 && cr4().c.PVI)) && !m_rflags.c.VIP) {
        m_rflags.c.VIF = 1;
    } else {
        return raise_integral_interrupt(Exceptions::GP(ZERO_ERROR_CODE_NOEXT));
    }
    return INCREMENT_IP;
} //	Set Interrupt Flag
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_SUB(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto second_val = MAY_HAVE_RAISED(second_op.read());
    if (second_op.operand().type == X86_OP_IMM)
        second_val = sign_extend(second_val, first_val.byte_width());
    auto res = CPUE_checked_single_usub(first_val, second_val);

    update_rflags(res);
    MAY_HAVE_RAISED(first_op.write(res.value));

    return INCREMENT_IP;
} //	Subtract
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_SWAPGS(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(do_privileged_instruction_check());
    TODO();
} //	Swap GS Base Register
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_TEST(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto second_val = sign_extend(MAY_HAVE_RAISED(second_op.read()), first_val.byte_width());

    auto anded_value = first_val & second_val;

    // The OF and CF flags are set to 0. The SF, ZF, and PF flags are set according to the result (see the “Operation” section above). The state of the AF flag is undefined.
    m_rflags.c.OF = m_rflags.c.CF = 0;
    update_rflags(anded_value);

    return INCREMENT_IP;
} //	Logical Compare
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_XCHG(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto second_val = MAY_HAVE_RAISED(second_op.read());

    MAY_HAVE_RAISED(first_op.write(second_val));
    MAY_HAVE_RAISED(second_op.write(first_val));
    //XCHG does not change a flag.

    return INCREMENT_IP;

} // Exchange
InterruptRaisedOr<CPU::IPIncrementBehavior> CPU::handle_XOR(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto second_val = MAY_HAVE_RAISED(second_op.read());
    if (second_op.operand().type == X86_OP_IMM)
        second_val = sign_extend(second_val, first_val.byte_width());

    auto xored_value = first_val ^ second_val;

    // The OF and CF flags are cleared; the SF, ZF, and PF flags are set according to the result. The state of the AF flag is undefined.
    m_rflags.c.OF = m_rflags.c.CF = 0;
    update_rflags(xored_value);

    MAY_HAVE_RAISED(first_op.write(xored_value));
    return INCREMENT_IP;
} // XOR


}
