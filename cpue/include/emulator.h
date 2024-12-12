#pragma once

#include "kernel.h"
#include "elf.h"
#include <string>
#include "cpu.h"

namespace CPUE {

class Emulator {
public:
    Emulator(Kernel& kernel, std::string const& binary_path, bool verbose) : m_kernel(kernel), m_binary(binary_path), m_verbose(verbose){};

    void start() const;
    // void single_step();

private:
    Kernel& m_kernel;
    ELF m_binary;
    bool m_verbose;
};


}