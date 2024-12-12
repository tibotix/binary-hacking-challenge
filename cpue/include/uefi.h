#pragma once

#include "cpu.h"

namespace CPUE {


class UEFI {
public:
    explicit UEFI(CPU* cpu) : m_cpu(cpu) {}

    void prepare_long_mode();

private:
    CPU* m_cpu;
};

}