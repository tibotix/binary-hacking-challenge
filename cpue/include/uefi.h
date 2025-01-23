#pragma once

#include "common.h"
#include "cpu.h"
#include "loader.h"
#include "paging.h"

namespace CPUE {


class UEFI {
public:
    explicit UEFI(CPU* cpu) : m_cpu(cpu) {}

    void prepare_long_mode(u64& top);

private:
    void setup_paging(u64& top);
    void setup_gdt(u64& top);
    void setup_idt(u64& top);
    void setup_stack(u64& top);
    void reserve_scratch_space(u64& top);

    CPU* m_cpu;
    Loader m_loader{*m_cpu};
};

}