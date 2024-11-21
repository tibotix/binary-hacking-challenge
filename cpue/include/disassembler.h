#pragma once

#include <capstone/capstone.h>


namespace CPUE {


// Forward Declaration
class CPU;


class Disassembler {
public:
    Disassembler(CPU* cpu) : m_cpu{cpu} {
        cs_open(CS_ARCH_X86, CS_MODE_64, &m_handle);
        m_insn = cs_malloc(m_handle);
    };
    Disassembler(Disassembler const&) = delete;
    ~Disassembler() { cs_free(m_insn, 1); }

public:
    cs_insn const* next_insn_or_null() const;

private:
    CPU* m_cpu;
    cs_insn* m_insn;
    csh m_handle;
};


}