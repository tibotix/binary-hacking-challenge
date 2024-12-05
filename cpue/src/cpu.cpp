
#include "cpu.h"
#include "tss.h"


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
        if (auto insn = m_disassembler.next_insn(); !insn.raised()) {
            m_state = STATE_HANDLE_INSTRUCTION;
            TODO_NOFAIL("Handle insn");

            // handle_insn()
        }

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
            (void)handle_interrupt(i);
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
    assert_in_long_mode();

    // See chapter 7.12 or page 3290
    m_interrupt_to_be_handled = interrupt;


    /**
     * The processor handles calls to exception- and interrupt-handlers similar to the way it handles calls with a CALL
     * instruction to a procedure or a task. When responding to an exception or interrupt, the processor uses the exception
     * or interrupt vector as an index to a descriptor in the IDT. If the index points to an interrupt gate or trap gate,
     * the processor calls the exception or interrupt handler in a manner similar to a CALL to a call gate (see Section
     * 6.8.2, “Gate Descriptors,” through Section 6.8.6, “Returning from a Called Procedure”). If index points to a task
     * gate, the processor executes a task switch to the exception- or interrupt-handler task in a manner similar to a CALL
     * to a task gate (see Section 9.3, “Task Switching”).
     */


    auto idt_descriptor = MAY_HAVE_RAISED(m_mmu.interrupt_vector_to_descriptor(interrupt.vector));
    switch (idt_descriptor.access.descriptor_type()) {
        case DescriptorType::TASK_GATE: MAY_HAVE_RAISED(enter_task_gate(interrupt, *idt_descriptor.to_task_gate_descriptor())); break;
        case DescriptorType::INTERRUPT_GATE:
        case DescriptorType::TRAP_GATE: MAY_HAVE_RAISED(enter_interrupt_trap_gate(interrupt, *idt_descriptor.to_trap_gate_descriptor())); break;
        // TODO: maybe raise exception here instead of fail
        default: fail("Invalid segment type for interrupt.");
    }

    // Only real exceptions push an error_code
    if (interrupt.type.category() == InterruptCategory::EXCEPTION) {
        if (interrupt.error_code.has_value()) {
            auto error_code = interrupt.error_code.value();
            MAY_HAVE_RAISED(stack_push(error_code.value));
        }
    }

    m_interrupt_to_be_handled.reset();
    return {};
}


InterruptRaisedOr<void> CPU::enter_interrupt_trap_gate(Interrupt const& i, TrapGateDescriptor const& descriptor) {
    ErrorCode error_code = {.standard = {
                                .tbl = 1,
                                .selector_index = i.vector,
                            }};
    // protection checks
    // rpl is not checked as vectors have none (only segment-selectors)
    // The processor checks the DPL of the interrupt or trap gate only if an exception or interrupt is generated with an
    // INT n, INT3, or INTO instruction (INT1 would not check). Here, the CPL must be less than or equal to the DPL of the gate.
    if (i.source == InterruptSource::INTN_INT3_INTO_INSN && cpl() > descriptor.access.dpl)
        return raise_interrupt(Exceptions::GP(error_code));

    auto dest_segment_descriptor = MAY_HAVE_RAISED(m_mmu.segment_selector_to_descriptor(descriptor.segment_selector));

    // Check if target code segment is executable code segment
    if (!dest_segment_descriptor.is_application_segment_descriptor() || dest_segment_descriptor.access.executable == 0)
        return raise_interrupt(Exceptions::GP(error_code));
    auto* dest_code_segment_descriptor = dest_segment_descriptor.to_application_segment_descriptor();
    // Target code segments referenced by a 64-bit call gate must be 64-bit code segments (CS.L = 1, CS.D = 0).
    if (dest_code_segment_descriptor->l == 0 || dest_code_segment_descriptor->db == 1)
        return raise_interrupt(Exceptions::GP(error_code));

    auto dest_is_conforming = dest_segment_descriptor.access.ec == 1;
    auto dest_dpl = dest_segment_descriptor.access.dpl;

    TODO_NOFAIL("Privilege Checks");

    /**
     * If a call is made to a more privileged (numerically lower privilege level) nonconforming destination code segment,
     * the CPL is lowered to the DPL of the destination code segment and a stack switch occurs (see Section 6.8.5, “Stack
     * Switching”). If a call or jump is made to a more privileged conforming destination code segment, the CPL is not
     * changed and no stack switch occurs.
     */
    auto old_ss = m_ss.visible.segment_selector;
    auto old_sp = m_rsp;
    if (dest_is_conforming && dest_dpl < cpl()) {
        std::tie(old_ss, old_sp) = MAY_HAVE_RAISED(do_stack_switch(dest_dpl));
    }

    // In 64-bit mode:
    // The stack pointer (SS:RSP) is pushed unconditionally on interrupts. In legacy modes, this push is conditional
    // and based on a change in current privilege level(CPL).
    MAY_HAVE_RAISED(stack_push(raw_bytes<u16>(&old_ss)));
    MAY_HAVE_RAISED(stack_push(old_sp));
    MAY_HAVE_RAISED(stack_push(raw_bytes<u64>(&m_rflags)));

    MAY_HAVE_RAISED(stack_push(raw_bytes<u16>(&m_cs.visible.segment_selector)));
    MAY_HAVE_RAISED(stack_push(m_rip));
    // Load the segment selector for the new code segment and the new instruction pointer from the call gate into
    // the CS and RIP registers, respectively, and begin execution of the called procedure.
    MAY_HAVE_RAISED(load_segment_register(SegmentRegisterAlias::CS, descriptor.segment_selector, dest_segment_descriptor));
    m_rip = dest_segment_descriptor.base() + descriptor.offset();

    /**
     * When accessing an exception or interrupt handler through either an interrupt gate or a trap gate, the processor
     * clears the TF flag in the EFLAGS register after it saves the contents of the EFLAGS register on the stack. (On calls
     * to exception and interrupt handlers, the processor also clears the VM, RF, and NT flags in the EFLAGS register, after
     * they are saved on the stack.) Clearing the TF flag prevents instruction tracing from affecting interrupt response and
     * ensures that no single-step exception will be delivered after delivery to the handler. A subsequent IRET instruction
     * restores the TF (and VM, RF, and NT) flags to the values in the saved contents of the EFLAGS register on the stack
     */
    m_rflags.TF = m_rflags.VM = m_rflags.RF = m_rflags.NT = 0;

    /**
     * The only difference between an interrupt gate and a trap gate is the way the processor handles the IF flag in the
     * EFLAGS register. When accessing an exception- or interrupt-handling procedure through an interrupt gate, the
     * processor clears the IF flag to prevent other interrupts from interfering with the current interrupt handler. A subse-
     * quent IRET instruction restores the IF flag to its value in the saved contents of the EFLAGS register on the stack.
     * Accessing a handler procedure through a trap gate does not affect the IF flag.
     */
    if (descriptor.access.descriptor_type() == DescriptorType::INTERRUPT_GATE) {
        m_rflags.IF = 0;
    }

    return {};
}


InterruptRaisedOr<void> CPU::enter_task_gate(Interrupt const& i, TaskGateDescriptor const& task_gate_descriptor) {
    TODO("enter_task_gate");
    // The processor issues a general-protection exception (#GP) if the following is attempted in 64-bit mode:
    // Control transfer to a TSS or a task gate using JMP, CALL, INT n, INT3, INTO, INT1, or interrupt.
}

InterruptRaisedOr<void> CPU::enter_call_gate(SegmentSelector const& selector, CallGateDescriptor const& call_gate_descriptor, bool through_call_insn) {
    assert_in_long_mode();

    ErrorCode error_code = {.standard = {
                                .tbl = (u8)(selector.table << 1),
                                .selector_index = selector.index,
                            }};
    // CPL ≤ call gate DPL; RPL ≤ call gate DPL
    if (cpl() > call_gate_descriptor.access.dpl || selector.rpl > call_gate_descriptor.access.dpl)
        return raise_interrupt(Exceptions::GP(error_code));

    auto dest_segment_descriptor = MAY_HAVE_RAISED(m_mmu.segment_selector_to_descriptor(call_gate_descriptor.segment_selector));

    // Check if target code segment is executable code segment
    if (!dest_segment_descriptor.is_application_segment_descriptor() || dest_segment_descriptor.access.executable == 0)
        return raise_interrupt(Exceptions::GP(error_code));
    auto* dest_code_segment_descriptor = dest_segment_descriptor.to_application_segment_descriptor();
    // Target code segments referenced by a 64-bit call gate must be 64-bit code segments (CS.L = 1, CS.D = 0).
    if (dest_code_segment_descriptor->l == 0 || dest_code_segment_descriptor->db == 1)
        return raise_interrupt(Exceptions::GP(error_code));

    auto dest_is_conforming = dest_segment_descriptor.access.ec == 1;
    auto dest_dpl = dest_segment_descriptor.access.dpl;
    // Destination conforming code segment DPL ≤ CPL
    if (dest_is_conforming && dest_dpl > cpl())
        return raise_interrupt(Exceptions::GP(error_code));
    if (!dest_is_conforming) {
        // CALL: Destination nonconforming code segment DPL ≤ CPL
        if (through_call_insn && dest_dpl > cpl())
            return raise_interrupt(Exceptions::GP(error_code));
        // JMP: Destination nonconforming code segment DPL = CPL
        if (!through_call_insn && dest_dpl != cpl())
            return raise_interrupt(Exceptions::GP(error_code));
    }

    /**
     * If a call is made to a more privileged (numerically lower privilege level) nonconforming destination code segment,
     * the CPL is lowered to the DPL of the destination code segment and a stack switch occurs (see Section 6.8.5, “Stack
     * Switching”). If a call or jump is made to a more privileged conforming destination code segment, the CPL is not
     * changed and no stack switch occurs.
     */
    if (dest_is_conforming && dest_dpl < cpl()) {
        auto [old_ss, old_sp] = MAY_HAVE_RAISED(do_stack_switch(dest_dpl));
        MAY_HAVE_RAISED(stack_push(raw_bytes<u16>(&old_ss)));
        MAY_HAVE_RAISED(stack_push(old_sp));
    }

    MAY_HAVE_RAISED(stack_push(raw_bytes<u16>(&m_cs.visible.segment_selector)));
    MAY_HAVE_RAISED(stack_push(m_rip));
    // Load the segment selector for the new code segment and the new instruction pointer from the call gate into
    // the CS and RIP registers, respectively, and begin execution of the called procedure.
    MAY_HAVE_RAISED(load_segment_register(SegmentRegisterAlias::CS, call_gate_descriptor.segment_selector, dest_segment_descriptor));
    m_rip = dest_segment_descriptor.base() + call_gate_descriptor.offset();
    return {};
}


InterruptRaisedOr<std::pair<SegmentSelector, u64>> CPU::do_stack_switch(u8 target_pl) {
    // Check TSS limits
    if (m_tr.hidden.cached_descriptor.limit() < sizeof(TSS) - 1) {
        ErrorCode error_code = {.standard = {.tbl = 0, .selector_index = m_tr.visible.segment_selector.index}};
        return raise_interrupt(Exceptions::TS(error_code));
    }
    // Stack switch
    // When a procedure call through a call gate results in a change in privilege level, the processor performs the following
    // steps to switch stacks and begin execution of the called procedure at a new privilege level :
    //  1. Uses the DPL of the destination code segment (the new CPL) to select a pointer to the new stack (segment
    //     selector and stack pointer) from the TSS.
    auto tss_sp_byte_offset = [&] -> u16 {
        switch (target_pl) {
            case 0: return offsetof(TSS, rsp0_1);
            case 1: return offsetof(TSS, rsp1_1);
            case 2: return offsetof(TSS, rsp2_1);
            default: fail("cpl>2 but TSS has only rsp0-2.");
        }
    }();
    //  2. Reads the segment selector and stack pointer for the stack to be switched to from the current TSS. Any limit
    //     violations detected while reading the stack-segment selector, stack pointer, or stack-segment descriptor cause
    //     an invalid TSS (#TS) exception to be generated.
    // -> When stacks are switched as part of a 64-bit mode privilege-level change through a call gate, a new SS (stack
    //    segment) descriptor is not loaded; 64-bit mode only loads an inner-level RSP from the TSS.
    auto new_sp_addr = VirtualAddress(m_tr.hidden.cached_descriptor.base() + (tss_sp_byte_offset * 8));
    TODO_NOFAIL("Check limit");
    auto new_sp = MAY_HAVE_RAISED(mmu().mem_read64(new_sp_addr, INTENTION_LOAD_TSS));
    new_sp = (new_sp << 32) || new_sp & 0xFFFFFFFF; // switch endianness
    //  3. Checks the stack-segment descriptor for the proper privileges and type and generates an invalid TSS (#TS)
    //     exception if violations are detected.
    // -> in 64-bit mode, we don't load a new SS and therefore also no stack-segment descriptor.
    //  4. Temporarily saves the current values of the SS and RSP registers.
    auto old_ss = m_ss;
    auto old_sp = m_rsp;
    // The new SS is forced to NULL and the SS selector’s RPL field is forced to the new CPL.
    // we can do this cause in 64-bit mode the processor does not perform runtime NULL-selector checks
    m_ss.visible.segment_selector.index = 0x0;
    m_ss.visible.segment_selector.rpl = target_pl;
    m_rsp = new_sp;
    //  6. Pushes the temporarily saved values for the SS and RSP registers (for the calling procedure) onto the new stack
    //  7. Copies the number of parameter specified in the parameter count field of the call gate from the calling
    //     procedure’s stack to the new stack. If the count is 0, no parameters are copied.
    // -> in 64-bit mode, this count is always 0.
    //  8. Pushes the return instruction pointer (the current contents of the CS and RIP registers) onto the new stack.
    return std::make_pair(old_ss.visible.segment_selector, old_sp);
}


template<typename... Args>
_InterruptRaised CPU::raise_interrupt(Interrupt i, Args&&... args) {
    // #PF and #CP use custom error_code format
    if (i.vector != Exceptions::VEC_PF && i.vector != Exceptions::VEC_CP && i.error_code.has_value()) {
        // don't clear already set ext bit, but otherwise use value of source (or 0, if we're currently not handling an interrupt)
        i.error_code->standard.ext |= m_interrupt_to_be_handled.has_value() ? m_interrupt_to_be_handled->source.is_external() : 0;
    }

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
    GDTLDTDescriptor descriptor = MAY_HAVE_RAISED(mmu().segment_selector_to_descriptor(selector));
    return load_segment_register(alias, selector, descriptor);
}

InterruptRaisedOr<void> CPU::load_segment_register(SegmentRegisterAlias alias, SegmentSelector selector, GDTLDTDescriptor const& descriptor) {
    ErrorCode error_code = {.standard = {
                                .tbl = static_cast<u8>(selector.table << 1),
                                .selector_index = selector.index,
                            }};

    // 64-bit mode does not perform NULL-selector runtime checking
    // If an attempt is made to load null selector in the SS register in CPL3 and 64-bit mode.
    if (selector.index == 0 && alias == SegmentRegisterAlias::SS && cpl() == 3) {
        return raise_integral_interrupt(Exceptions::GP(error_code));
    }

    // The accessed bit indicates whether the segment has been accessed since the last time the operating-system or
    // executive cleared the bit. The processor sets this bit whenever it loads a segment selector for the segment into a
    // segment register, assuming that the type of memory that contains the segment descriptor supports processor
    // writes. (Can also be used for example to track number of accesses to a descriptor).
    auto [base, limit] = descriptor_table_of_selector(selector);
    auto const access_byte_vaddr = VirtualAddress(base) + (selector.index * 8) + offsetof(Descriptor, access);
    auto new_access_byte = descriptor.access;
    new_access_byte.accessed = 1;
    MAY_HAVE_RAISED(mmu().mem_write8(access_byte_vaddr, raw_bytes<u8>(&new_access_byte)));

    // The Segment Not Present exception occurs when trying to load a segment or gate which has its `Present` bit set to 0.
    // However, when loading a stack-segment selector which references a descriptor which is not present, a Stack-Segment Fault occurs.
    if (descriptor.access.present == 0) {
        if (alias.type() == SegmentRegisterType::STACK) {
            return raise_integral_interrupt(Exceptions::SS(error_code));
        }
        return raise_integral_interrupt(Exceptions::NP(error_code));
    }

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
    if (descriptor.access.is_system_descriptor() && alias.type() != SegmentRegisterType::DATA) {
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
            // A transfer into a nonconforming segment at a different privilege level results in a general-protection exception (#GP),
            // unless a gate is used to access it.
            if (descriptor.access.dpl != cpl() && !descriptor.is_gate_descriptor()) {
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
    return {};
}



InterruptRaisedOr<void> CPU::stack_push(u64 value) {
    TODO_NOFAIL("maybe make general and do alignment checks (in MMU)");
    if (m_rsp < 8)
        return raise_integral_interrupt(Exceptions::SS(ZERO_ERROR_CODE_NOEXT));
    m_rsp -= 8;
    MAY_HAVE_RAISED(mmu().mem_write64(stack_pointer(), value));
    return {};
}

InterruptRaisedOr<u64> CPU::stack_pop() {
    TODO("stack_pop");
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

DescriptorTable CPU::descriptor_table_of_selector(SegmentSelector selector) const {
    switch (selector.table) {
        case 0: return m_gdtr;
        case 1: return {m_ldtr.hidden.cached_descriptor.base(), m_ldtr.hidden.cached_descriptor.limit()};
        default: fail();
    }
}

void CPU::init_mmio() {}


}
