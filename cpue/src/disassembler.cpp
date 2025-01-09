
#include "disassembler.h"
#include "cpu.h"

namespace CPUE {

// Reading instructions byte by byte
InterruptRaisedOr<cs_insn const> Disassembler::next_insn() {
    u64 address = m_cpu->m_rip;
    u8 code[20] = {0};
    u64 code_size = 1;

    for (; code_size <= sizeof(code) - 1; ++code_size) {
        code[code_size] = MAY_HAVE_RAISED(m_cpu->mmu().mem_read8(LogicalAddress(m_cpu->m_cs, address + code_size - 1)));
        u8 const* code_ptr = code;
        if (cs_disasm_iter(m_handle, &code_ptr, &code_size, &address, m_insn))
            return *m_insn;
    }
    return m_cpu->raise_interrupt(Exceptions::UD());
}

}
