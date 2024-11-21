
#include "cpu.h"


namespace CPUE {


void CPU::interpreter_loop() {
    for (;;) {
        cs_insn const* insn = m_disassembler.next_insn_or_null();
        TODO_NOFAIL("Handle insn");


        auto interrupt = m_pic.pending_interrupt_with_highest_priority();
        if (interrupt.has_value()) {
            TODO_NOFAIL("Handle interrupt");
        }
    }
}

InterruptRaisedOr<void> CPU::do_canonicality_check(VirtualAddress const& vaddr) {
    auto high_bits = vaddr.addr & ~VIRTUAL_ADDR_MASK;
    if (high_bits != 0 && high_bits != ~VIRTUAL_ADDR_MASK) {
        // An access to memory using a linear address is allowed only if the address is paging canonical; if it is not, a canonicality violation occurs.
        // In most cases, an access causing a canonicality violation results in a general protection exception (#GP);
        // for stack accesses (those due to stack-oriented instructions, as well as accesses that implicitly or
        // explicitly use the SS segment register), a stack fault (#SS) is generated. In either case, a null error code is
        // produced.
        TODO_NOFAIL("raise #GP or #SS");
        return m_pic.deliver_interrupt(Exceptions::GP);
    }
    return {};
}

}