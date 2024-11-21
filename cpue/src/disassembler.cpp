
#include "disassembler.h"
#include "cpu.h"

namespace CPUE {


cs_insn const* Disassembler::next_insn_or_null() const {
    u8* code_ptr = m_cpu->mmu().va_to_pa(m_cpu.m_rip);
    size_t code_size = 3;
    u64 address = m_rip;

    TODO("fetch next insn");
    TODO("update rip");

    // disassemble one instruction a time & store the result into @insn variable above
    if (cs_disasm_iter(handle, &code, &code_size, &address, m_insn)) {
        return m_insn;
    }
    return NULL;
}

}