
#include "cpu.h"
#include <sstream>

namespace CPUE {

// TODO: use dispatch array, and build it in constexpr, use nullptr for not implemented and pointer to member function otherwise
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_insn(cs_insn const& insn) {
    CPUE_TRACE("Handling instruction: {} {}", insn.mnemonic, insn.op_str);
    auto const& detail = insn.detail->x86;
#define CASE(name) \
    case x86_insn::X86_INS_##name: return handle_##name(detail); break;

    switch (insn.id) {
        CASE(ADD)
        CASE(AND)
        CASE(CALL)
        CASE(CLI)
        CASE(CLD)
        CASE(CMP)
        CASE(CMPXCHG)
        CASE(CWD)
        CASE(CDQ)
        CASE(CQO)
        CASE(DEC)
        CASE(DIV)
        CASE(ENDBR64)
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
        CASE(JA)
        CASE(JAE)
        CASE(JNE)
        CASE(JE)
        CASE(JGE)
        CASE(JG)
        CASE(JLE)
        CASE(JL)
        CASE(JB)
        CASE(JBE)
        CASE(LEA)
        CASE(LEAVE)
        CASE(LGDT)
        CASE(LIDT)
        CASE(LLDT)
        CASE(LTR)
        CASE(LOOP)
        CASE(LOOPE)
        CASE(LOOPNE)
        CASE(MOV)
        CASE(MOVABS)
        CASE(MOVSX)
        CASE(MOVSXD)
        CASE(MOVZX)
        CASE(MOVQ)
        CASE(MOVSB)
        CASE(MOVSW)
        CASE(MOVSD)
        CASE(MOVSQ)
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
        CASE(SETE)
        CASE(SETNE)
        CASE(SETL)
        CASE(SETG)
        CASE(SETGE)
        CASE(SETB)
        CASE(SIDT)
        CASE(SLDT)
        CASE(STI)
        CASE(STC)
        CASE(STD)
        CASE(STOSB)
        CASE(STOSW)
        CASE(STOSD)
        CASE(STOSQ)
        CASE(SUB)
        CASE(SWAPGS)
        CASE(TEST)
        CASE(XCHG)
        CASE(XOR)

        default: {
            std::stringstream ss;
            ss << "Unhandled instruction: " << insn.mnemonic;
            TODO(ss.str().c_str());
        }
    }
#undef CASE
}


/**
 * Helper Functions:
 */

template<typename Func>
requires std::is_same_v<std::invoke_result_t<Func>, InterruptRaisedOr<CPU::IPContinuationBehavior>> InterruptRaisedOr<CPU::IPContinuationBehavior>
    CPU::do_string_op_and_handle_rep_prefixes(RepPrefix prefix, cs_x86 const& insn_detail, Func&& do_op) {
    if (prefix == REP_PREFIX_NONE) {
        return do_op();
    }

    // handle REP prefix case
    auto should_terminate = [&]() -> bool {
        switch (prefix) {
            case REP_PREFIX_NONE:
            case REP_PREFIX_REP: return false;
            case REP_PREFIX_REPE: return m_rflags.c.ZF == 0;
            case REP_PREFIX_REPNE: return m_rflags.c.ZF == 1;
            default: return false;
        }
    };
    // TODO: test this with golden file
    auto count_reg_alias = [&]() -> x86_reg {
        switch (insn_detail.addr_size) {
            case 2: return X86_REG_CX;
            case 4: return X86_REG_ECX;
            case 8: return X86_REG_RCX;
            default: fail();
        }
    }();
    auto count_reg = reg(count_reg_alias);

    auto ip_cont = CONTINUE_IP;
    auto count = MAY_HAVE_RAISED(count_reg->read());
    ;
    while (count != 0x0) {
        // TODO: Service pending interrupts (if any)
        ip_cont = MAY_HAVE_RAISED(do_op());
        MAY_HAVE_RAISED(count_reg->write(--count));
        if (should_terminate())
            break;
    }

    return ip_cont;
}
InterruptRaisedOr<void> CPU::do_privileged_instruction_check(u8 pl) {
    if (cpl() != pl)
        return raise_integral_interrupt(Exceptions::GP(ZERO_ERROR_CODE_NOEXT));
    return {};
}


/**
 * Handler Implementations:
 */

InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_ADD(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto second_val = MAY_HAVE_RAISED(second_op.read());
    if (second_op.operand().type == X86_OP_IMM)
        second_val = second_val.sign_extended_to_width(first_val.byte_width());
    auto res = CPUE_checked_single_uadd(first_val, second_val);

    update_rflags(res);
    MAY_HAVE_RAISED(first_op.write(res.value));

    return CONTINUE_IP;
} //	Add
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_AND(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto second_val = MAY_HAVE_RAISED(second_op.read()).sign_extended_to_width(first_val.byte_width());

    auto anded_value = first_val & second_val;
    MAY_HAVE_RAISED(first_op.write(anded_value));

    // The OF and CF flags are cleared; the SF, ZF, and PF flags are set according to the result. The state of the AF flag is undefined.
    m_rflags.c.OF = m_rflags.c.CF = 0;
    update_rflags(anded_value);

    return CONTINUE_IP;
} //	Logical AND
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_CALL(cs_x86 const& insn_detail) {
    // Only near and far calls are allowed in 64-bit mode -> we don't need to consider inter-privilege-far-calls and task-switches.
    assert_in_64bit_mode();
    TODO_NOFAIL("Check for far calls");

    auto first_op = Operand(this, insn_detail.operands[0]);

    MAY_HAVE_RAISED(stack_push(m_rip_val));
    m_rip_val = MAY_HAVE_RAISED(first_op.read()).value();
    return CONTINUE_IP;
} //    Call Procedure
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_CLI(cs_x86 const& insn_detail) {
    if (cr0().c.PE) {
        m_rflags.c.IF = 0;
    } else if (m_rflags.c.IOPL >= cpl()) {
        m_rflags.c.IF = 0;
    } else if (/*VME Mode*/ (m_rflags.c.VM && cr4().c.VME) || /*PVI Mode*/ (!m_rflags.c.VM && cpl() == 3 && cr4().c.PVI)) {
        m_rflags.c.VIF = 0;
    } else {
        return raise_integral_interrupt(Exceptions::GP(ZERO_ERROR_CODE_NOEXT));
    }
    return CONTINUE_IP;
} //    Clear Interrupt Flag
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_CLD(cs_x86 const& insn_detail) {
    m_rflags.c.DF = 0;
    return CONTINUE_IP;
} //    Clear Direction Flag
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_CMP(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto second_val = MAY_HAVE_RAISED(second_op.read());
    if (second_op.operand().type == X86_OP_IMM)
        second_val = second_val.sign_extended_to_width(first_val.byte_width());
    auto res = CPUE_checked_single_usub(first_val, second_val);

    update_rflags(res);

    return CONTINUE_IP;
} //    Compare Two Operands
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_CMPXCHG(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    auto accumulator_reg = [&]() {
        switch (first_op.byte_width()) {
            case ByteWidth::WIDTH_BYTE: return X86_REG_AL;
            case ByteWidth::WIDTH_WORD: return X86_REG_AX;
            case ByteWidth::WIDTH_DWORD: return X86_REG_EAX;
            case ByteWidth::WIDTH_QWORD: return X86_REG_RAX;
            default: fail();
        }
    }();
    auto accumulator = reg(accumulator_reg);
    auto accumulator_val = MAY_HAVE_RAISED(accumulator->read());
    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto res = CPUE_checked_single_usub(accumulator_val, first_val);

    update_rflags(res);

    if (m_rflags.c.ZF) {
        auto second_val = MAY_HAVE_RAISED(second_op.read());
        MAY_HAVE_RAISED(first_op.write(second_val));
    } else {
        MAY_HAVE_RAISED(accumulator->write(first_val));
    }

    return CONTINUE_IP;
} //    Compare and Exchange
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_CWD(cs_x86 const& insn_detail) {
    // DX:AX := sign-extend of AX.
    auto value = MAY_HAVE_RAISED(reg(X86_REG_AX)->read()).sign_extended_to_width(ByteWidth::WIDTH_DWORD);
    MAY_HAVE_RAISED(reg(X86_REG_DX)->write(value.upper_half()));
    MAY_HAVE_RAISED(reg(X86_REG_AX)->write(value.lower_half()));
    return CONTINUE_IP;
} //    Convert Word to Doubleword/Convert Doubleword to Quadword
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_CDQ(cs_x86 const& insn_detail) {
    // EDX:EAX := sign-extend of EAX.
    auto value = MAY_HAVE_RAISED(reg(X86_REG_EAX)->read()).sign_extended_to_width(ByteWidth::WIDTH_QWORD);
    MAY_HAVE_RAISED(reg(X86_REG_EDX)->write(value.upper_half()));
    MAY_HAVE_RAISED(reg(X86_REG_EAX)->write(value.lower_half()));
    return CONTINUE_IP;
} //    Convert Word to Doubleword/Convert Doubleword to Quadword
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_CQO(cs_x86 const& insn_detail) {
    // RDX:RAX:= sign-extend of RAX.
    auto value = MAY_HAVE_RAISED(reg(X86_REG_RAX)->read()).sign_extended_to_width(ByteWidth::WIDTH_DQWORD);
    MAY_HAVE_RAISED(reg(X86_REG_RDX)->write(value.upper_half()));
    MAY_HAVE_RAISED(reg(X86_REG_RAX)->write(value.lower_half()));
    return CONTINUE_IP;
} //    Convert Word to Doubleword/Convert Doubleword to Quadword
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_DEC(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);

    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto res = CPUE_checked_single_usub(first_val, SizedValue(1));

    // The CF flag is not affected. The OF, SF, ZF, AF, and PF flags are set according to the result.
    m_rflags.c.OF = res.has_of_set;
    update_rflags(res.value);
    MAY_HAVE_RAISED(first_op.write(res.value));

    return CONTINUE_IP;
} //    Decrement By 1
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_DIV_IDIV(x86_insn const& insn, cs_x86 const& insn_detail) {
    bool is_signed_division = insn == X86_INS_IDIV;
    auto first_op = Operand(this, insn_detail.operands[0]);

    auto quotient_dest = [&]() -> x86_reg {
        switch (first_op.byte_width()) {
            case ByteWidth::WIDTH_BYTE: return X86_REG_AL;
            case ByteWidth::WIDTH_WORD: return X86_REG_AX;
            case ByteWidth::WIDTH_DWORD: return X86_REG_EAX;
            case ByteWidth::WIDTH_QWORD: return X86_REG_RAX;
            default: fail();
        }
    }();
    auto quotient_dest_reg = reg(quotient_dest);
    auto quotient_reg_width = quotient_dest_reg->byte_width();
    auto remainder_dest = [&]() -> x86_reg {
        switch (first_op.byte_width()) {
            case ByteWidth::WIDTH_BYTE: return X86_REG_AH;
            case ByteWidth::WIDTH_WORD: return X86_REG_DX;
            case ByteWidth::WIDTH_DWORD: return X86_REG_EDX;
            case ByteWidth::WIDTH_QWORD: return X86_REG_RDX;
            default: fail();
        }
    }();
    auto remainder_dest_reg = reg(remainder_dest);
    auto remainder_reg_width = remainder_dest_reg->byte_width();

    // divisor = remainder_reg:quotient_reg
    auto dividend = (MAY_HAVE_RAISED(remainder_dest_reg->read()).zero_extended_to_width(quotient_reg_width.double_width()) << quotient_reg_width.bit_width()) |
                    MAY_HAVE_RAISED(quotient_dest_reg->read());
    if (is_signed_division)
        dividend = dividend.sign_extended_to_width(ByteWidth::WIDTH_DQWORD);
    auto divisor = MAY_HAVE_RAISED(first_op.read());
    if (is_signed_division)
        divisor = divisor.sign_extended_to_width(ByteWidth::WIDTH_DQWORD);
    if (divisor == 0)
        return raise_integral_interrupt(Exceptions::DE());
    auto do_div = [&]<typename T>() -> T {
        return dividend.as<T>() / divisor.as<T>();
    };
    auto quotient = SizedValue((is_signed_division ? do_div.operator()<i128>() : do_div.operator()<u128>()), dividend.byte_width());

    // store quotient, and if it is too large for designated destination register, throw DE
    auto truncated_quotient = quotient.truncated_to_width(quotient_reg_width);
    if ((!is_signed_division && quotient != truncated_quotient) || (is_signed_division && quotient != truncated_quotient.sign_extended_to_width(quotient.byte_width())))
        return raise_integral_interrupt(Exceptions::DE());
    MAY_HAVE_RAISED(quotient_dest_reg->write(truncated_quotient));

    // store remainder
    auto do_mod = [&]<typename T>() -> T {
        return dividend.as<T>() % divisor.as<T>();
    };
    auto remainder = SizedValue((is_signed_division ? do_mod.operator()<i128>() : do_mod.operator()<u128>()), remainder_reg_width);
    MAY_HAVE_RAISED(remainder_dest_reg->write(remainder));

    return CONTINUE_IP;
}
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_DIV(cs_x86 const& insn_detail) {
    return handle_DIV_IDIV(X86_INS_DIV, insn_detail);
} //	Unsigned Divide
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_ENDBR64(cs_x86 const& insn_detail) {
    // NOTE: we do not support CET yet
    return CONTINUE_IP;
} //    Terminate an Indirect Branch in 64-bit Mode
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_HLT(cs_x86 const& insn_detail) {
    CPUE_INFO("encountered a HLT instruction. We use this instruction to exit the emulator (although normally it would behave quite differently)");
    shutdown();
} //	Halt
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_IDIV(cs_x86 const& insn_detail) {
    return handle_DIV_IDIV(X86_INS_IDIV, insn_detail);
} //	Signed Divide
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_IMUL(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);

    auto lower_dest = [&]() -> x86_reg {
        if (insn_detail.op_count == 1) {
            switch (first_op.byte_width()) {
                case ByteWidth::WIDTH_BYTE: return X86_REG_AX;
                case ByteWidth::WIDTH_WORD: return X86_REG_AX;
                case ByteWidth::WIDTH_DWORD: return X86_REG_EAX;
                case ByteWidth::WIDTH_QWORD: return X86_REG_RAX;
                default: fail();
            }
        } else {
            return first_op.operand().reg;
        }
    }();
    auto lower_dest_reg = reg(lower_dest);
    auto upper_dest = [&]() -> x86_reg {
        if (insn_detail.op_count == 1) {
            switch (first_op.byte_width()) {
                case ByteWidth::WIDTH_BYTE: return X86_REG_INVALID;
                case ByteWidth::WIDTH_WORD: return X86_REG_DX;
                case ByteWidth::WIDTH_DWORD: return X86_REG_EDX;
                case ByteWidth::WIDTH_QWORD: return X86_REG_RDX;
                default: fail();
            }
        } else {
            return X86_REG_INVALID;
        }
    }();
    auto upper_dest_reg = reg(upper_dest);

    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto second_val = insn_detail.op_count > 1 ? MAY_HAVE_RAISED(Operand(this, insn_detail.operands[1]).read()) : SizedValue(0);
    // When an immediate value is used as an operand, it is sign-extended to the length of the destination operand format.
    auto third_val = insn_detail.op_count > 2 ? MAY_HAVE_RAISED(Operand(this, insn_detail.operands[2]).read()).sign_extended_to_width(lower_dest_reg->byte_width()) : SizedValue(0);

    auto factor1 = [&]() -> SizedValue {
        switch (insn_detail.op_count) {
            case 1: return {m_rax_val, first_op.byte_width()};
            case 2: return first_val;
            case 3: return second_val;
            default: fail();
        }
    }();
    auto factor2 = [&]() -> SizedValue {
        switch (insn_detail.op_count) {
            case 1: return first_val;
            case 2: return second_val;
            case 3: return third_val;
            default: fail();
        }
    }();

    auto res = CPUE_checked_single_imul(factor1, factor2);

    if (upper_dest == X86_REG_INVALID) {
        MAY_HAVE_RAISED(lower_dest_reg->write(res.value.truncated_to_width(lower_dest_reg->byte_width())));
    } else {
        MAY_HAVE_RAISED(lower_dest_reg->write(res.value.lower_half()));
        MAY_HAVE_RAISED(upper_dest_reg->write(res.value.upper_half()));
    }

    update_rflags_cf_of(res);
    return CONTINUE_IP;
} //	Signed Multiply
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_INC(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);

    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto res = CPUE_checked_single_uadd(first_val, SizedValue(1));

    // The CF flag is not affected. The OF, SF, ZF, AF, and PF flags are set according to the result.
    m_rflags.c.OF = res.has_of_set;
    update_rflags(res.value);
    MAY_HAVE_RAISED(first_op.write(res.value));

    return CONTINUE_IP;
} //	Increment by 1
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_INT(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    Interrupt i = {
        .vector = MAY_HAVE_RAISED(first_op.read()).as<InterruptVector>(),
        .type = InterruptType::SOFTWARE_INTERRUPT,
        .iclass = InterruptClass::BENIGN,
        .source = InterruptSource::INTN_INT3_INTO_INSN,
    };

    MAY_HAVE_RAISED(handle_interrupt(i));
    return CONTINUE_IP;
} // 	Call to Interrupt Procedure
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_INT1(cs_x86 const& insn_detail) {
    static Interrupt i = {
        .vector = 1,
        .type = InterruptType::SOFTWARE_INTERRUPT,
        .iclass = InterruptClass::BENIGN,
        .source = InterruptSource::INT1_INSN,
    };
    MAY_HAVE_RAISED(handle_interrupt(i));
    return CONTINUE_IP;
} //	Call to Interrupt Procedure
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_INT3(cs_x86 const& insn_detail) {
    // TODO: only for debugging
    __asm__("int3");
    return CONTINUE_IP;
    static Interrupt i = {
        .vector = 3,
        .type = InterruptType::SOFTWARE_INTERRUPT,
        .iclass = InterruptClass::BENIGN,
        .source = InterruptSource::INTN_INT3_INTO_INSN,
    };
    MAY_HAVE_RAISED(handle_interrupt(i));
    return CONTINUE_IP;
} //	Call to Interrupt Procedure
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_INTO(cs_x86 const& insn_detail) {
    static Interrupt i = {
        .vector = 4,
        .type = InterruptType::SOFTWARE_INTERRUPT,
        .iclass = InterruptClass::BENIGN,
        .source = InterruptSource::INTN_INT3_INTO_INSN,
    };
    if (m_rflags.c.OF) {
        MAY_HAVE_RAISED(handle_interrupt(i));
    }
    return CONTINUE_IP;
} //	Call to Interrupt Procedure
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_INVLPG(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(do_privileged_instruction_check());

    auto first_op = Operand(this, insn_detail.operands[0]);
    auto addr = MAY_HAVE_RAISED(operand_mem_offset(first_op.operand().mem));

    m_mmu.tlb().invalidate(addr);

    return CONTINUE_IP;
} //	Invalidate TLB Entries
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_IRET_IRETD_IRETQ(x86_insn const& insn, cs_x86 const& insn_detail) {
    /*
     * TODO: (When implementing NMIs)
     * If nonmaskable interrupts (NMIs) are blocked (see Section 6.7.1, “Handling Multiple NMIs” in the Intel® 64 and IA-32 Architectures Software Developer’s Manual, Volume 3A),
     * execution of the IRET instruction unblocks NMIs.
     */
    MAY_HAVE_RAISED(do_privileged_instruction_check());
    assert_in_64bit_mode();

    RFLAGS temp_rflags;

    auto operand_byte_size = [&]() -> u64 {
        // NOTE: As we know for sure that we're in 64-bit mode, we don't need to check cs.d/l bits
        switch (insn) {
            // In 64-bit mode, the instruction’s default operation size is 32 bits.
            case X86_INS_IRET:
            case X86_INS_IRETD: return 4;
            case X86_INS_IRETQ: return 8;
            default: fail();
        }
    }();
    auto operand_byte_mask = bytemask(operand_byte_size);

    auto pop_ip_cs = [&]() -> InterruptRaisedOr<void> {
        m_rip_val = MAY_HAVE_RAISED(stack_pop()) & operand_byte_mask;
        auto cs = MAY_HAVE_RAISED(stack_pop()) & 0xFFFF;
        MAY_HAVE_RAISED(load_segment_register(SegmentRegisterAlias::CS, cs));
        CPUE_ASSERT(execution_mode() == ExecutionMode::IA32e_64BIT_MODE, "We currently do not support an iret to a non 64-bit mode execution.");
        return {};
    };

    auto pop_temp_rflags = [&]() -> InterruptRaisedOr<void> {
        temp_rflags = {.value = MAY_HAVE_RAISED(stack_pop())};
        return {};
    };

    auto pop_sp_ss = [&]() -> InterruptRaisedOr<void> {
        auto rsp = MAY_HAVE_RAISED(stack_pop()) & operand_byte_mask;
        auto ss = MAY_HAVE_RAISED(stack_pop()) & 0xFFFF;
        // Now all values have been popped, we can safely change rsp
        m_rsp_val = rsp;
        return load_segment_register(SegmentRegisterAlias::SS, ss);
    };

    auto load_rflags = [&]() -> void {
        m_rflags.c.CF = temp_rflags.c.CF;
        m_rflags.c.PF = temp_rflags.c.PF;
        m_rflags.c.AF = temp_rflags.c.AF;
        m_rflags.c.ZF = temp_rflags.c.ZF;
        m_rflags.c.SF = temp_rflags.c.SF;
        m_rflags.c.TF = temp_rflags.c.TF;
        m_rflags.c.DF = temp_rflags.c.DF;
        m_rflags.c.OF = temp_rflags.c.OF;
        m_rflags.c.NT = temp_rflags.c.NT;
        if (operand_byte_size == 4 || operand_byte_size == 8) {
            m_rflags.c.RF = temp_rflags.c.RF;
            m_rflags.c.AC = temp_rflags.c.AC;
            m_rflags.c.ID = temp_rflags.c.ID;
        }
        if (cpl() <= m_rflags.c.IOPL)
            m_rflags.c.IF = temp_rflags.c.IF;
        if (cpl() == 0) {
            m_rflags.c.IOPL = temp_rflags.c.IOPL;
            if (operand_byte_size == 4 || operand_byte_size == 8) {
                m_rflags.c.VIF = temp_rflags.c.VIF;
                m_rflags.c.VIP = temp_rflags.c.VIP;
            }
        }
    };

    auto exec_mode = execution_mode();
    switch (exec_mode) {
        case REAL_MODE: goto real_mode;
        case PROTECTED_MODE: goto protected_mode;
        case IA32e_COMPATIBILITY_MODE:
        case IA32e_64BIT_MODE: goto long_mode;
    }

real_mode:
    TODO("iret real-mode.");
protected_mode:
    TODO("iret protected-mode");
return_from_vm8086_mode:
    TODO("iret vm8086 mode");
return_to_outer_privilege_level:
    MAY_HAVE_RAISED(pop_sp_ss());
    for (auto r : {X86_REG_ES, X86_REG_FS, X86_REG_GS, X86_REG_DS}) {
        auto sr = application_segment_register(r).value();
        // if (SegmentSelector == NULL) OR (tempDesc(DPL) < CPL AND tempDesc(Type) is (data or non-conforming code)))
        if (sr->visible.segment_selector.value == 0x0 ||
            (sr->hidden.cached_descriptor.access.c.dpl < cpl() &&
                (sr->hidden.cached_descriptor.access.descriptor_type() == DescriptorType::DATA_SEGMENT ||
                    (sr->hidden.cached_descriptor.access.descriptor_type() == DescriptorType::CODE_SEGMENT && sr->hidden.cached_descriptor.access.c.ec == 0)))) {
            // SegmentSelector = 0x0
            sr->visible.segment_selector.value = 0x0;
        }
    }
    goto end;

return_to_same_privilege_level:
    goto end;

long_mode:
    if (m_rflags.c.NT)
        return raise_integral_interrupt(Exceptions::GP(ZERO_ERROR_CODE_NOEXT));
    MAY_HAVE_RAISED(pop_ip_cs());
    // NOTE: we assert new-mode == 64-bit mode in pop_ip_cs()
    MAY_HAVE_RAISED(do_canonicality_check(m_rip_val));

    MAY_HAVE_RAISED(pop_temp_rflags());
    load_rflags();

    if (m_cs.visible.segment_selector.c.rpl > cpl()) {
        goto return_to_outer_privilege_level;
    }
    if (exec_mode == ExecutionMode::IA32e_64BIT_MODE) {
        MAY_HAVE_RAISED(pop_sp_ss());
    }
    goto return_to_same_privilege_level;

end:
    return CONTINUE_IP;
}
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_IRET(cs_x86 const& insn_detail) {
    return handle_IRET_IRETD_IRETQ(X86_INS_IRET, insn_detail);
} //	Interrupt Return
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_IRETD(cs_x86 const& insn_detail) {
    return handle_IRET_IRETD_IRETQ(X86_INS_IRETD, insn_detail);
} //	Interrupt Return
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_IRETQ(cs_x86 const& insn_detail) {
    return handle_IRET_IRETD_IRETQ(X86_INS_IRETQ, insn_detail);
} //	Interrupt Return
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_JMP(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);

    TODO_NOFAIL("Check for far jumps.");
    m_rip_val = MAY_HAVE_RAISED(first_op.read()).value();
    return CONTINUE_IP;
} // Jump
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_JA(cs_x86 const& insn_detail) {
    if (!m_rflags.c.CF && !m_rflags.c.ZF)
        return handle_JMP(insn_detail);
    return CONTINUE_IP;
} //	Jump Above
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_JAE(cs_x86 const& insn_detail) {
    if (!m_rflags.c.CF)
        return handle_JMP(insn_detail);
    return CONTINUE_IP;
} //	Jump Above or Equal
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_JNE(cs_x86 const& insn_detail) {
    if (!m_rflags.c.ZF)
        return handle_JMP(insn_detail);
    return CONTINUE_IP;
} //	Jump Not Equal
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_JE(cs_x86 const& insn_detail) {
    if (m_rflags.c.ZF)
        return handle_JMP(insn_detail);
    return CONTINUE_IP;
} //	Jump Equal
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_JGE(cs_x86 const& insn_detail) {
    if (m_rflags.c.SF == m_rflags.c.OF)
        return handle_JMP(insn_detail);
    return CONTINUE_IP;
} //	Jump Greater or Equal
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_JG(cs_x86 const& insn_detail) {
    if (!m_rflags.c.ZF && m_rflags.c.SF == m_rflags.c.OF)
        return handle_JMP(insn_detail);
    return CONTINUE_IP;
} //	Jump Greater
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_JLE(cs_x86 const& insn_detail) {
    if (m_rflags.c.ZF || m_rflags.c.SF != m_rflags.c.OF)
        return handle_JMP(insn_detail);
    return CONTINUE_IP;
} //	Jump Lower or Equal
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_JL(cs_x86 const& insn_detail) {
    if (m_rflags.c.SF != m_rflags.c.OF)
        return handle_JMP(insn_detail);
    return CONTINUE_IP;
} //	Jump Lower
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_JB(cs_x86 const& insn_detail) {
    if (m_rflags.c.CF)
        return handle_JMP(insn_detail);
    return CONTINUE_IP;
} //	Jump Below
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_JBE(cs_x86 const& insn_detail) {
    if (m_rflags.c.CF || m_rflags.c.ZF)
        return handle_JMP(insn_detail);
    return CONTINUE_IP;
} //	Jump Below or Equal
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_LEA(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    if (second_op.operand().type != X86_OP_MEM)
        return raise_integral_interrupt(Exceptions::UD());
    auto offset = MAY_HAVE_RAISED(operand_mem_offset(second_op.operand().mem));
    offset &= bytemask(insn_detail.addr_size);

    MAY_HAVE_RAISED(first_op.write(SizedValue(offset, first_op.byte_width())));

    return CONTINUE_IP;
} //	Load Effective Address
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_LEAVE(cs_x86 const& insn_detail) {
    m_rsp_val = m_rbp_val;
    m_rbp_val = MAY_HAVE_RAISED(stack_pop());

    return CONTINUE_IP;
} //	High Level Procedure Exit
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_LGDT(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(do_privileged_instruction_check());
    TODO();
} //	Load Global/Interrupt Descriptor Table Register
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_LIDT(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(do_privileged_instruction_check());
    TODO();
} //	Load Global/Interrupt Descriptor Table Register
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_LLDT(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(do_privileged_instruction_check());
    TODO();
} //	Load Local Descriptor Table Register
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_LTR(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(do_privileged_instruction_check());
    TODO();
} //	Load Task Register
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_LOOP_LOOPE_LOOPNE(x86_insn const& insn, cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);

    auto ctr_reg_alias = [&]() -> x86_reg {
        switch (insn_detail.addr_size) {
            case 8: return X86_REG_RCX;
            case 4: return X86_REG_ECX;
            default: return X86_REG_CX;
        }
    }();
    auto ctr_reg = reg(ctr_reg_alias);

    auto new_ctr_val = MAY_HAVE_RAISED(ctr_reg->read()) - 1;
    MAY_HAVE_RAISED(ctr_reg->write(new_ctr_val));

    auto should_branch = [&]() -> bool {
        switch (insn) {
            case X86_INS_LOOP: return new_ctr_val != 0;
            case X86_INS_LOOPE: return new_ctr_val != 0 && m_rflags.c.ZF;
            case X86_INS_LOOPNE: return new_ctr_val != 0 && !m_rflags.c.ZF;
            default: fail();
        }
    }();
    if (should_branch) {
        auto dest_rip = MAY_HAVE_RAISED(first_op.read()).value();
        MAY_HAVE_RAISED(do_canonicality_check(dest_rip));
        m_rip_val = dest_rip;
    }
    return CONTINUE_IP;
}
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_LOOP(cs_x86 const& insn_detail) {
    return handle_LOOP_LOOPE_LOOPNE(X86_INS_LOOP, insn_detail);
} //	Loop According to ECX Counter
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_LOOPE(cs_x86 const& insn_detail) {
    return handle_LOOP_LOOPE_LOOPNE(X86_INS_LOOPE, insn_detail);
} //	Loop According to ECX Counter
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_LOOPNE(cs_x86 const& insn_detail) {
    return handle_LOOP_LOOPE_LOOPNE(X86_INS_LOOPNE, insn_detail);
} //	Loop According to ECX Counter
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_MOV(cs_x86 const& insn_detail) {
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
        second_val = second_val.sign_extended_to_width(first_op.byte_width());

    MAY_HAVE_RAISED(first_op.write(second_val));

    return CONTINUE_IP;
} //	Move
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_MOVABS(cs_x86 const& insn_detail) {
    return handle_MOV(insn_detail);
} //	Move With Sign-Extension
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_MOVSX(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]); // Destination
    auto second_op = Operand(this, insn_detail.operands[1]); // Source

    auto second_val = MAY_HAVE_RAISED(second_op.read()).sign_extended_to_width(first_op.byte_width());

    MAY_HAVE_RAISED(first_op.write(second_val));

    return CONTINUE_IP;
} //	Move With Sign-Extension
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_MOVSXD(cs_x86 const& insn_detail) {
    return handle_MOVSX(insn_detail);
} //	Move With Sign-Extension
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_MOVZX(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]); // Destination
    auto second_op = Operand(this, insn_detail.operands[1]); // Source

    auto second_val = MAY_HAVE_RAISED(second_op.read()).zero_extended_to_width(first_op.byte_width());

    MAY_HAVE_RAISED(first_op.write(second_val));

    return CONTINUE_IP;
} //	Move With Zero-Extend
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_MOVQ(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    auto second_val = MAY_HAVE_RAISED(second_op.read()).zero_extended_or_truncated_to_width(first_op.byte_width());

    MAY_HAVE_RAISED(first_op.write(second_val));
    return CONTINUE_IP;
} //    Move Quadword
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_MOVSB_MOVSW_MOVSD_MOVSQ(x86_insn const& insn, cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);
    CPUE_ASSERT(first_op.operand().type == X86_OP_MEM && second_op.operand().type == X86_OP_MEM, "MOVS with non-memory operands.");
    CPUE_ASSERT(first_op.operand().size == second_op.operand().size, "MOVS with mismatching memory operand sizes.");

    auto do_op = [&]() -> InterruptRaisedOr<IPContinuationBehavior> {
        auto second_val = MAY_HAVE_RAISED(second_op.read());
        MAY_HAVE_RAISED(first_op.write(second_val));

        auto inc = first_op.operand().size * ((-2 * m_rflags.c.DF) + 1);

        auto first_reg = gpreg(first_op.operand().mem.base);
        auto first_reg_val = MAY_HAVE_RAISED(first_reg->read());
        MAY_HAVE_RAISED(first_reg->write(first_reg_val + inc));

        auto second_reg = gpreg(second_op.operand().mem.base);
        auto second_reg_val = MAY_HAVE_RAISED(second_reg->read());
        MAY_HAVE_RAISED(second_reg->write(second_reg_val + inc));
        return CONTINUE_IP;
    };

    // MOVS can only have REP prefix.
    auto prefix = insn_detail.prefix[0] == X86_PREFIX_REP ? REP_PREFIX_REP : REP_PREFIX_NONE;
    return do_string_op_and_handle_rep_prefixes(prefix, insn_detail, do_op);
} //     Move Data From String to String
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_MOVSB(cs_x86 const& insn_detail) {
    return handle_MOVSB_MOVSW_MOVSD_MOVSQ(X86_INS_MOVSB, insn_detail);
} //     Move Data From String to String
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_MOVSW(cs_x86 const& insn_detail) {
    return handle_MOVSB_MOVSW_MOVSD_MOVSQ(X86_INS_MOVSW, insn_detail);
} //     Move Data From String to String
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_MOVSD(cs_x86 const& insn_detail) {
    return handle_MOVSB_MOVSW_MOVSD_MOVSQ(X86_INS_MOVSD, insn_detail);
} //     Move Data From String to String
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_MOVSQ(cs_x86 const& insn_detail) {
    return handle_MOVSB_MOVSW_MOVSD_MOVSQ(X86_INS_MOVSQ, insn_detail);
} //     Move Data From String to String
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_MUL(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto first_val = MAY_HAVE_RAISED(first_op.read());

    auto lower_dest_reg = [&]() -> x86_reg {
        switch (first_val.byte_width()) {
            case ByteWidth::WIDTH_BYTE: return X86_REG_AX;
            case ByteWidth::WIDTH_WORD: return X86_REG_AX;
            case ByteWidth::WIDTH_DWORD: return X86_REG_EAX;
            case ByteWidth::WIDTH_QWORD: return X86_REG_RAX;
            default: fail();
        }
    }();
    auto upper_dest_reg = [&]() -> x86_reg {
        switch (first_val.byte_width()) {
            case ByteWidth::WIDTH_BYTE: return X86_REG_INVALID;
            case ByteWidth::WIDTH_WORD: return X86_REG_DX;
            case ByteWidth::WIDTH_DWORD: return X86_REG_EDX;
            case ByteWidth::WIDTH_QWORD: return X86_REG_RDX;
            default: fail();
        }
    }();

    auto factor = SizedValue(m_rax_val, first_val.byte_width());
    auto res = CPUE_checked_single_umul(first_val, factor);

    if (upper_dest_reg == X86_REG_INVALID) {
        MAY_HAVE_RAISED(reg(lower_dest_reg)->write(res.value));
    } else {
        MAY_HAVE_RAISED(reg(lower_dest_reg)->write(res.value.lower_half()));
        MAY_HAVE_RAISED(reg(upper_dest_reg)->write(res.value.upper_half()));
    }

    update_rflags_cf_of(res);
    return CONTINUE_IP;
} //	Unsigned Multiply
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_NOP(cs_x86 const& insn_detail) {
    return CONTINUE_IP;
} //	No Operation
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_NOT(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);

    auto value = MAY_HAVE_RAISED(first_op.read());
    auto not_value = ~value;

    MAY_HAVE_RAISED(first_op.write(not_value));

    return CONTINUE_IP;
} //	One's Complement Negation
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_OR(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto second_val = MAY_HAVE_RAISED(second_op.read()).sign_extended_to_width(first_val.byte_width());

    auto ored_value = first_val | second_val;
    MAY_HAVE_RAISED(first_op.write(ored_value));

    // The OF and CF flags are cleared; the SF, ZF, and PF flags are set according to the result. The state of the AF flag is undefined.
    m_rflags.c.OF = m_rflags.c.CF = 0;
    update_rflags(ored_value);

    return CONTINUE_IP;
} //	Logical Inclusive OR
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_POP(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);

    auto value = MAY_HAVE_RAISED(stack_pop());

    MAY_HAVE_RAISED(first_op.write(SizedValue(value)));

    return CONTINUE_IP;
} //	Pop a Value From the Stack
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_POPF(cs_x86 const& insn_detail) {
    TODO();
} //	Pop Stack Into lower 16 bits of RFLAGS Register
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_POPFQ(cs_x86 const& insn_detail) {
    TODO();
} //	Pop Stack Into RFLAGS Register
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_PUSH(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);

    auto value = MAY_HAVE_RAISED(first_op.read());

    MAY_HAVE_RAISED(stack_push(value.value()));
    return CONTINUE_IP;
} //	Push Word, Doubleword, or Quadword Onto the Stack
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_PUSHF(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(stack_push(m_rflags.value & 0x000000000000FFFF));
    return CONTINUE_IP;
} //	Push lower 16 bits of RFLAGS Register Onto the Stack
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_PUSHFQ(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(stack_push(m_rflags.value & 0x0000000000FCFFFF));
    return CONTINUE_IP;
} //	Push RFLAGS Register Onto the Stack
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_RET(cs_x86 const& insn_detail) {
    // Only near and far ret are allowed in 64-bit mode -> we don't need to consider inter-privilege-far-ret.
    assert_in_64bit_mode();

    auto value = MAY_HAVE_RAISED(stack_pop());
    m_rip_val = value;

    // Release parameters from stack if operand is given
    if (insn_detail.op_count == 1) {
        auto first_op = Operand(this, insn_detail.operands[0]);
        m_rsp_val += MAY_HAVE_RAISED(first_op.read()).value();
    }

    TODO_NOFAIL("Check for far ret");
    // When executing a far return, the processor pops the return instruction pointer from the top of the stack into the EIP register,
    // then pops the segment selector from the top of the stack into the CS register.
    // The processor then begins program execution in the new code segment at the new instruction pointer.

    return CONTINUE_IP;
} //	Return From Procedure
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_ROL(cs_x86 const& insn_detail) {
    TODO();
} //	Rotate
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_ROR(cs_x86 const& insn_detail) {
    TODO();
} //	Rotate
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_SAL(cs_x86 const& insn_detail) {
    return handle_SAL_SAR_SHL_SHR(X86_INS_SAL, insn_detail);
} //	Shift
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_SAR(cs_x86 const& insn_detail) {
    return handle_SAL_SAR_SHL_SHR(X86_INS_SAR, insn_detail);
} //	Shift
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_SGDT(cs_x86 const& insn_detail) {
    TODO();
} //	Store Global Descriptor Table Register
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_SAL_SAR_SHL_SHR(x86_insn const& insn, cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    // The count is masked to 5 bits (or 6 bits with a 64-bit operand). The count range is limited to 0 to 31 (or 63 with a 64-bit operand).
    u8 count_mask = [&]() -> u8 {
        switch (first_op.byte_width()) {
            case ByteWidth::WIDTH_QWORD: return 0x3F;
            default: return 0x1F;
        }
    }();
    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto shift_num = MAY_HAVE_RAISED(second_op.read()) & count_mask;

    if (shift_num == 0)
        return CONTINUE_IP;

    auto is_left_shift = one_of(insn, {X86_INS_SAL, X86_INS_SHL});
    auto do_shift = [&](SizedValue const& value, u8 count) -> SizedValue {
        if (is_left_shift) {
            return value << count;
        }
        // The SHR instruction clears the most significant bit
        if (insn == X86_INS_SHR) {
            return value >> count;
        }
        // the SAR instruction sets or clears the most significant bit to correspond to the sign (most significant bit)
        // of the original value in the destination operand
        if (insn == X86_INS_SAR) {
            u64 extension = value.msb() ? (1 << count) - 1 : 0;
            return (value >> count) | extension;
        }
        fail();
    };
    auto temp_res = do_shift(first_val, shift_num.as<u8>() - 1);
    m_rflags.c.CF = is_left_shift ? temp_res.msb() : temp_res.lsb();
    auto res = do_shift(temp_res, 1);

    MAY_HAVE_RAISED(first_op.write(res));

    if (shift_num == 1)
        m_rflags.c.OF = [&]() -> u8 {
            switch (insn) {
                case X86_INS_SAL:
                case X86_INS_SHL: return res.msb() ^ m_rflags.c.CF;
                case X86_INS_SAR: return 0;
                case X86_INS_SHR: return first_val.msb();
                default: return 0;
            }
        }();

    // The SF, ZF, and PF flags are set according to the result. If the count is 0, the flags are not affected.
    update_rflags(res);

    return CONTINUE_IP;
}
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_SHL(cs_x86 const& insn_detail) {
    return handle_SAL_SAR_SHL_SHR(X86_INS_SHL, insn_detail);
} //	Shift
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_SHLD(cs_x86 const& insn_detail) {
    TODO();
} //	Double Precision Shift Left
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_SHLX(cs_x86 const& insn_detail) {
    TODO();
} //	Shift Without Affecting Flags
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_SHR(cs_x86 const& insn_detail) {
    return handle_SAL_SAR_SHL_SHR(X86_INS_SHR, insn_detail);
} //	Shift
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_SETcc(x86_insn const& insn, cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto condition = [&]() -> bool {
        switch (insn) {
            case X86_INS_SETE: return m_rflags.c.ZF;
            case X86_INS_SETNE: return !m_rflags.c.ZF;
            case X86_INS_SETL: return m_rflags.c.SF != m_rflags.c.OF;
            case X86_INS_SETG: return !m_rflags.c.ZF && m_rflags.c.SF == m_rflags.c.OF;
            case X86_INS_SETGE: return m_rflags.c.SF = m_rflags.c.OF;
            case X86_INS_SETB: return m_rflags.c.CF;
            default: fail();
        }
    }();
    MAY_HAVE_RAISED(first_op.write(SizedValue(condition, ByteWidth::WIDTH_BYTE)));
    return CONTINUE_IP;
} //    Set Byte on Condition
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_SETE(cs_x86 const& insn_detail) {
    return handle_SETcc(X86_INS_SETE, insn_detail);
}
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_SETNE(cs_x86 const& insn_detail) {
    return handle_SETcc(X86_INS_SETNE, insn_detail);
}
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_SETL(cs_x86 const& insn_detail) {
    return handle_SETcc(X86_INS_SETL, insn_detail);
}
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_SETG(cs_x86 const& insn_detail) {
    return handle_SETcc(X86_INS_SETG, insn_detail);
}
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_SETGE(cs_x86 const& insn_detail) {
    return handle_SETcc(X86_INS_SETGE, insn_detail);
}
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_SETB(cs_x86 const& insn_detail) {
    return handle_SETcc(X86_INS_SETB, insn_detail);
}
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_SIDT(cs_x86 const& insn_detail) {
    if (cr4().c.UMIP)
        MAY_HAVE_RAISED(do_privileged_instruction_check());

    auto first_op = Operand(this, insn_detail.operands[0]);
    auto addr = VirtualAddress(MAY_HAVE_RAISED(operand_mem_offset(first_op.operand().mem)));

    MAY_HAVE_RAISED(m_mmu.mem_write16(addr, m_idtr.limit));
    MAY_HAVE_RAISED(m_mmu.mem_write64(addr + 2, m_idtr.base));

    return CONTINUE_IP;
} //	Store Interrupt Descriptor Table Register
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_SLDT(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(do_privileged_instruction_check());
    TODO();
} //	Store Local Descriptor Table Register
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_STI(cs_x86 const& insn_detail) {
    /**
    * TODO:
    * Page 3286:
    * > Maskable hardware interrupts remain inhibited on the instruction boundary following an execution of STI.
    * > The inhibition ends after delivery of another event (e.g., exception) or the execution of the next instruction.
    */
    if (cr0().c.PE) {
        m_rflags.c.IF = 1;
    } else if (m_rflags.c.IOPL >= cpl()) {
        m_rflags.c.IF = 1;
    } else if ((/*VME Mode*/ (m_rflags.c.VM && cr4().c.VME) || /*PVI Mode*/ (!m_rflags.c.VM && cpl() == 3 && cr4().c.PVI)) && !m_rflags.c.VIP) {
        m_rflags.c.VIF = 1;
    } else {
        return raise_integral_interrupt(Exceptions::GP(ZERO_ERROR_CODE_NOEXT));
    }
    return CONTINUE_IP;
} //	Set Interrupt Flag
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_STC(cs_x86 const& insn_detail) {
    m_rflags.c.CF = 1;
    return CONTINUE_IP;
}
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_STD(cs_x86 const& insn_detail) {
    m_rflags.c.DF = 1;
    return CONTINUE_IP;
}
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_STOSB_STOSW_STOSD_STOSQ(x86_insn const& insn, cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]); // this is populated by capstone
    auto second_op = Operand(this, insn_detail.operands[0]); // this is populated by capstone
    CPUE_ASSERT(first_op.operand().type == X86_OP_MEM, "STOS with non-memory operand.");

    auto do_op = [&]() -> InterruptRaisedOr<IPContinuationBehavior> {
        auto second_val = MAY_HAVE_RAISED(second_op.read());
        MAY_HAVE_RAISED(first_op.write(second_val));

        auto inc = first_op.operand().size * ((-2 * m_rflags.c.DF) + 1);

        auto first_reg = gpreg(first_op.operand().mem.base);
        auto first_reg_val = MAY_HAVE_RAISED(first_reg->read());
        MAY_HAVE_RAISED(first_reg->write(first_reg_val + inc));

        return CONTINUE_IP;
    };

    // STOS can only have REP prefix.
    auto prefix = insn_detail.prefix[0] == X86_PREFIX_REP ? REP_PREFIX_REP : REP_PREFIX_NONE;
    return do_string_op_and_handle_rep_prefixes(prefix, insn_detail, do_op);
} //    Store String
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_STOSB(cs_x86 const& insn_detail) {
    return handle_STOSB_STOSW_STOSD_STOSQ(X86_INS_STOSB, insn_detail);
}
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_STOSW(cs_x86 const& insn_detail) {
    return handle_STOSB_STOSW_STOSD_STOSQ(X86_INS_STOSW, insn_detail);
}
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_STOSD(cs_x86 const& insn_detail) {
    return handle_STOSB_STOSW_STOSD_STOSQ(X86_INS_STOSD, insn_detail);
}
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_STOSQ(cs_x86 const& insn_detail) {
    return handle_STOSB_STOSW_STOSD_STOSQ(X86_INS_STOSQ, insn_detail);
}
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_SUB(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto second_val = MAY_HAVE_RAISED(second_op.read());
    if (second_op.operand().type == X86_OP_IMM)
        second_val = second_val.sign_extended_to_width(first_val.byte_width());
    auto res = CPUE_checked_single_usub(first_val, second_val);

    update_rflags(res);
    MAY_HAVE_RAISED(first_op.write(res.value));

    return CONTINUE_IP;
} //	Subtract
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_SWAPGS(cs_x86 const& insn_detail) {
    MAY_HAVE_RAISED(do_privileged_instruction_check());
    TODO();
} //	Swap GS Base Register
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_TEST(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto second_val = MAY_HAVE_RAISED(second_op.read()).sign_extended_to_width(first_val.byte_width());

    auto anded_value = first_val & second_val;

    // The OF and CF flags are set to 0. The SF, ZF, and PF flags are set according to the result (see the “Operation” section above). The state of the AF flag is undefined.
    m_rflags.c.OF = m_rflags.c.CF = 0;
    update_rflags(anded_value);

    return CONTINUE_IP;
} //	Logical Compare
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_XCHG(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto second_val = MAY_HAVE_RAISED(second_op.read());

    MAY_HAVE_RAISED(first_op.write(second_val));
    MAY_HAVE_RAISED(second_op.write(first_val));
    //XCHG does not change a flag.

    return CONTINUE_IP;

} // Exchange
InterruptRaisedOr<CPU::IPContinuationBehavior> CPU::handle_XOR(cs_x86 const& insn_detail) {
    auto first_op = Operand(this, insn_detail.operands[0]);
    auto second_op = Operand(this, insn_detail.operands[1]);

    auto first_val = MAY_HAVE_RAISED(first_op.read());
    auto second_val = MAY_HAVE_RAISED(second_op.read());
    if (second_op.operand().type == X86_OP_IMM)
        second_val = second_val.sign_extended_to_width(first_val.byte_width());

    auto xored_value = first_val ^ second_val;

    // The OF and CF flags are cleared; the SF, ZF, and PF flags are set according to the result. The state of the AF flag is undefined.
    m_rflags.c.OF = m_rflags.c.CF = 0;
    update_rflags(xored_value);

    MAY_HAVE_RAISED(first_op.write(xored_value));
    return CONTINUE_IP;
} // XOR


}
