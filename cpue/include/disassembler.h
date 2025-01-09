#pragma once

#include <capstone/capstone.h>
#include "forward.h"
#include "interrupts.h"


namespace CPUE {


class Disassembler {
public:
    friend class CPU;
    explicit Disassembler(CPU* cpu) : m_cpu{cpu} {
        cs_open(CS_ARCH_X86, CS_MODE_64, &m_handle);
        cs_option(m_handle, CS_OPT_DETAIL, CS_OPT_ON);
        m_insn = cs_malloc(m_handle);
    };
    Disassembler(Disassembler const&) = delete;
    ~Disassembler() { cs_free(m_insn, 1); }

public:
    InterruptRaisedOr<cs_insn const> next_insn();

private:
    CPU* m_cpu;
    cs_insn* m_insn;
    csh m_handle;
};


}
