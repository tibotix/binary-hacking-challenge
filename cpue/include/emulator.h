#pragma once

#include <cmath>
#include <string>
#include "kernel.h"
#include "elf.h"

namespace CPUE {

class Emulator {
public:
    Emulator(Kernel& kernel, std::string const& binary_path) : m_kernel(kernel), m_binary(binary_path){};

    void set_serial(bool serial) { m_serial = serial; };
    void set_verbosity(bool verbose) { m_verbose = verbose; };
    void set_available_ram_in_mb(size_t mb) { m_available_pages = std::ceil(mb * 1_mb / PAGE_SIZE); }

    void start();
    // void single_step();

private:
    Kernel& m_kernel;
    ELF m_binary;
    bool m_verbose = false;
    bool m_serial = true;
    size_t m_available_pages = 1024;
};


}