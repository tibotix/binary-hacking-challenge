
#include "cpu.h"


namespace CPUE {


static constexpr Descriptor::AccessByte _default_access_byte = {
    .accessed = 1,
    .wr = 1,
    .present = 1,
};
static constexpr ApplicationSegmentRegister _default_application_segment_register = {.visible = 0x0,
    .hidden = {.cached_descriptor = {.limit1 = 0xFFFF, .base1 = 0x0, .access = _default_access_byte, .limit2 = 0x0, .base2 = 0x0}}};
static constexpr ApplicationSegmentRegister _default_cs_segment_register = {.visible = 0x0,
    .hidden = {.cached_descriptor = {.limit1 = 0xF000, .base1 = 0xFFFF00, .access = _default_access_byte, .limit2 = 0x0, .base2 = 0x0}}};
static constexpr SystemSegmentRegister _default_system_segment_register = {.visible = 0x0,
    .hidden = {.cached_descriptor = {.limit1 = 0xFFFF, .base1 = 0x0, .access = _default_access_byte, .limit2 = 0x0, .base2 = 0x0, .base3 = 0x0}}};


void CPU::reset() {
    // See page 3425

    m_rax = 0x0;
    m_rbx = 0x0;
    m_rcx = 0x0;
    m_rdx = 0x00000600;
    m_rsi = 0x0;
    m_rsp = 0x0;
    m_rbp = 0x0;
    m_rip = 0x0000FFF0;
    m_r8 = m_r9 = m_r10 = m_r11 = m_r12 = m_r13 = m_r14 = m_r15 = 0x0;

    m_cs = _default_cs_segment_register;
    m_ds = m_ss = m_es = m_fs = m_gs = _default_application_segment_register;

    m_rflags = instance<RFLAGS, u64>(0x2);
    m_cr0 = instance<CR0, u64>(0x60000010);
    m_cr2 = 0x0_va;
    m_cr3 = instance<CR3, u64>(0x0);
    m_cr4 = instance<CR4, u64>(0x0);
    m_cr8 = instance<CR8, u64>(0x0);

    m_efer = instance<EFER, u64>(0x0);

    m_fsbase = m_gsbase = 0x0;
    m_kernel_gsbase = 0x0;

    m_gdtr = {.base = 0x00000000, .limit = 0xFFFF};
    m_idtr = {.base = 0x00000000, .limit = 0xFFFF};

    m_ldtr = m_tr = _default_system_segment_register;
};


void CPU::interpreter_loop() {
    for (;;) {
        m_state = STATE_FETCH_INSTRUCTION;
        cs_insn const* insn = m_disassembler.next_insn_or_null();

        m_state = STATE_HANDLE_INSTRUCTION;
        TODO_NOFAIL("Handle insn");
        // handle_insn()

        // Handle interrupts
        m_state = STATE_HANDLE_INTERRUPT;
        for (;;) {
            auto opt_i = m_icu.pop_highest_priority_interrupt();
            // if we don't have any interrupts left, break
            if (!opt_i.has_value())
                break;

            Interrupt i = opt_i.value();

            /**
             * The processor first services a pending event from the class which has the highest priority,
             * transferring execution to the first instruction of the handler.
             * Lower priority exceptions are discarded; lower priority interrupts are held pending.
             */

            if (i.type.category() == InterruptCategory::INTERRUPT) {
                // clear all exceptions in ICU, as they are guaranteed to be of lower priority than i
                // (since i is not an exception)
                TODO_NOFAIL("clear all exceptions in ICU");
            }
            TODO_NOFAIL("clear all exceptions having priority lower than i");

            /**
             * As we don't implement asynchronous interrupts, we know for sure, that when
             * handle_nested_interrupt and handle_interrupt return, they finished by either
             * succeeding or raising another interrupt.
             */

            if (m_interrupt_to_be_handled.has_value()) {
                if (handle_nested_interrupt(i).raised()) {
                    // we raised another exception here, go handle it.
                    // we also know for sure, that this interrupt finished, so mark it as handled.
                    m_interrupt_to_be_handled.reset();
                    continue;
                }
            }

            // It doesn't matter if this raises, because no matter what we always process all pending interrupts.
            handle_interrupt(i);
        }
    }
}


namespace {
enum _NestedInterruptAction {
    NIA_HANDLE_SERIALLY,
    NIA_GENERATE_DOUBLE_FAULT,
    NIA_SHUTDOWN,
};
typedef _NestedInterruptAction NIA;
static NIA _nested_interrupt_action_map[4][3] = {
    // First: Benign
    {NIA::NIA_HANDLE_SERIALLY, NIA::NIA_HANDLE_SERIALLY, NIA::NIA_HANDLE_SERIALLY},
    // First: Contributory
    {NIA::NIA_HANDLE_SERIALLY, NIA::NIA_GENERATE_DOUBLE_FAULT, NIA::NIA_HANDLE_SERIALLY},
    // First: PageFault
    {NIA::NIA_HANDLE_SERIALLY, NIA::NIA_GENERATE_DOUBLE_FAULT, NIA::NIA_GENERATE_DOUBLE_FAULT},
    // First: DoubleFault
    {NIA::NIA_HANDLE_SERIALLY, NIA::NIA_SHUTDOWN, NIA::NIA_SHUTDOWN},
};
}
InterruptRaisedOr<void> CPU::handle_nested_interrupt(Interrupt interrupt) {
    // NOTE: all interrupts and exceptions can be nested once their handlers are called
    /**
     * For example; if the CPU tries to start a page fault handler but can't because it triggers a second page fault;
     * then you get a double fault instead. However; if the CPU successfully starts the page fault handler then the
     * page fault handler's code can happily be interrupted by a second page fault.
     */

    // we received an interrupt while trying to call the interrupt handler for the previous interrupt
    // and are about to enter a nested interrupt scenario.
    u8 action = _nested_interrupt_action_map[m_interrupt_to_be_handled.value().iclass][interrupt.iclass];
    switch (action) {
        case NIA_HANDLE_SERIALLY: return {};
        case NIA_GENERATE_DOUBLE_FAULT: return raise_interrupt(Exceptions::DF(ZERO_ERROR_CODE_NOEXT));
        case NIA_SHUTDOWN: shutdown();
        default: fail();
    }
}

InterruptRaisedOr<void> CPU::handle_interrupt(Interrupt interrupt) {
    // See chapter 7.12 or page 3290
    m_interrupt_to_be_handled = interrupt;

    // TODO: If a vector references a descriptor beyond the limit of the IDT, a general-protection exception (#GP) is generated.

    // Only real exceptions push an error_code
    if (interrupt.type.category() == InterruptCategory::EXCEPTION) {
        if (interrupt.error_code.has_value()) {
            auto error_code = interrupt.error_code.value();
            TODO_NOFAIL("push error code onto stack");
        }
    }

    m_interrupt_to_be_handled.reset();
    TODO_NOFAIL("jump to interrupt handler");
    return {};
}


template<typename... Args>
_InterruptRaised CPU::raise_interrupt(Interrupt i, Args&&... args) {
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


InterruptRaisedOr<void> CPU::load_segment_register(SegmentRegisterAlias alias, SegmentSelector selector) {
    ErrorCode error_code = {.standard = {
                                .ext = 1,
                                .tbl = static_cast<u8>(selector.table << 1),
                                .selector_index = selector.index,
                            }};

    // 64-bit mode does not perform NULL-selector runtime checking
    // If an attempt is made to load null selector in the SS register in CPL3 and 64-bit mode.
    if (selector.index == 0 && alias == SegmentRegisterAlias::SS && cpl() == 3) {
        return raise_integral_interrupt(Exceptions::GP(error_code));
    }


    GDTLDTDescriptor descriptor = MAY_HAVE_RAISED(mmu().segment_selector_to_descriptor(selector));

    TODO_NOFAIL("set accessed bit");



    // The Segment Not Present exception occurs when trying to load a segment or gate which has its `Present` bit set to 0.
    // However when loading a stack-segment selector which references a descriptor which is not present, a Stack-Segment Fault occurs.
    if (alias.type() == SegmentRegisterType::STACK) {
        return raise_integral_interrupt(Exceptions::SS(error_code));
    }
    return raise_integral_interrupt(Exceptions::NP(error_code));



    // Type Checking (page 3250)
    // — The LDTR can only be loaded with a selector for an LDT.
    if (alias == SegmentRegisterAlias::LDTR && descriptor.access.descriptor_type() != DescriptorType::LDT_SEGMENT) {
        return raise_integral_interrupt(Exceptions::GP(error_code));
    }
    // — The task register can only be loaded with a segment selector for a TSS.
    if (alias == SegmentRegisterAlias::TR && !one_of(descriptor.access.descriptor_type(), {DescriptorType::TSS_AVAILABLE_SEGMENT, DescriptorType::TSS_BUSY_SEGMENT})) {
        return raise_integral_interrupt(Exceptions::GP(error_code));
    }
    // — The CS register only can be loaded with a selector for a code segment.
    if (alias == SegmentRegisterAlias::CS && descriptor.access.descriptor_type() != DescriptorType::CODE_SEGMENT) {
        return raise_integral_interrupt(Exceptions::GP(error_code));
    }
    // — Segment selectors for code segments that are not readable cannot be loaded into data-segment registers (DS, ES, FS, and GS).
    if (descriptor.access.descriptor_type() == DescriptorType::CODE_SEGMENT && !descriptor.access.wr && alias.type() == SegmentRegisterType::DATA) {
        return raise_integral_interrupt(Exceptions::GP(error_code));
    }
    // — Segment selectors for system segments cannot be loaded into data-segment registers (DS, ES, FS, and GS).
    // I think they mean:
    //  Segment selectors for system segments cannot be loaded into system-segment registers (DS, ES, FS, and GS).
    if (descriptor.access.is_system_descriptor() && alias.type() != SegmentRegisterType::SYSTEM) {
        return raise_integral_interrupt(Exceptions::GP(error_code));
    }
    // — Only segment selectors of writable data segments can be loaded into the SS register.
    if (alias == SegmentRegisterAlias::SS && descriptor.access.descriptor_type() == DescriptorType::DATA_SEGMENT && !descriptor.access.wr) {
        return raise_integral_interrupt(Exceptions::GP(error_code));
    }


    /**
     * Privilege Level Checking (chapter 6.5 or page 3252)
     * Privilege levels are checked when the segment selector of a segment descriptor is loaded into a segment register.
     * The checks used for data access differ from those used for transfers of program control among code segments;
     * therefore, the two kinds of accesses are considered separately in the following sections.
     */
    TODO_NOFAIL("Privilege Level Checking");
    if (descriptor.access.descriptor_type() == DescriptorType::DATA_SEGMENT) {
        // PRIVILEGE LEVEL CHECKING WHEN ACCESSING DATA SEGMENTS
        // The processor loads the segment selector into the segment register if the DPL is numerically greater
        // than or equal to both the CPL and the RPL. Otherwise, a general-protection fault is generated and the segment
        // register is not loaded.
        if (!(descriptor.access.dpl <= cpl() && descriptor.access.dpl <= selector.rpl)) {
            return raise_integral_interrupt(Exceptions::GP(error_code));
        }
    }
    if (alias == SegmentRegisterAlias::SS) {
        // PRIVILEGE LEVEL CHECKING WHEN LOADING THE SS REGISTER
        // If the RPL and DPL are not equal to the CPL, a general-protection exception (#GP) is generated.
        if (!(descriptor.access.dpl == cpl() == selector.rpl)) {
            return raise_integral_interrupt(Exceptions::GP(error_code));
        }
    }
    if (descriptor.access.descriptor_type() == DescriptorType::CODE_SEGMENT) {
        // PRIVILEGE LEVEL CHECKING WHEN TRANSFERRING PROGRAM CONTROL BETWEEN CODE SEGMENTS
        if (!descriptor.access.ec) {
            // Accessing Nonconforming Code Segments
            // When accessing nonconforming code segments, the CPL of the calling procedure must be equal to the DPL of the
            // destination code segment; otherwise, the processor generates a general-protection exception (#GP).
            if (!(descriptor.access.dpl == cpl())) {
                return raise_integral_interrupt(Exceptions::GP(error_code));
            }
        } else {
            // Accessing Conforming Code Segments
            // When accessing conforming code segments, the CPL of the calling procedure may be numerically equal to or
            // greater than (less privileged) the DPL of the destination code segment; the processor generates a general-protec-
            // tion exception (#GP) only if the CPL is less than the DPL. (The segment selector RPL for the destination code
            // segment is not checked if the segment is a conforming code segment.)
            if (cpl() < descriptor.access.dpl) {
                return raise_integral_interrupt(Exceptions::GP(error_code));
            }
        }
    }

    if (alias.type() == SegmentRegisterType::SYSTEM) {
        *m_system_segment_register_map[alias] = {
            .visible = selector,
            .hidden = *descriptor.to_system_segment_descriptor(),
        };
    } else {
        *m_segment_register_map[alias] = {
            .visible = selector,
            .hidden = *descriptor.to_application_segment_descriptor(),
        };
    }
}

InterruptRaisedOr<void> CPU::do_canonicality_check(VirtualAddress const& vaddr) {
    auto high_bits = vaddr.addr & ~VIRTUAL_ADDR_MASK;
    if (high_bits != 0 && high_bits != ~VIRTUAL_ADDR_MASK) {
        // Access to memory using a linear address is allowed only if the address is paging canonical; if it is not, a canonicality violation occurs.
        // In most cases, an access causing a canonicality violation results in a general protection exception (#GP);
        // for stack accesses (those due to stack-oriented instructions, as well as accesses that implicitly or
        // explicitly use the SS segment register), a stack fault (#SS) is generated. In either case, a null error code is
        // produced.
        TODO_NOFAIL("raise #GP or #SS");
        return raise_interrupt(Exceptions::GP(ZERO_ERROR_CODE_NOEXT));
    }
    return {};
}
}