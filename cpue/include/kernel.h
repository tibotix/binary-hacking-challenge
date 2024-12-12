#pragma once
#include "elf.h"


namespace CPUE {


enum KernelType {
    NONE,
    EMULATE,
    CUSTOM,
};

class Kernel {
public:
    Kernel() = default;
    virtual ~Kernel() = default;

    virtual bool init() = 0;
};

class NoKernel final : public Kernel {
public:
    NoKernel() = default;
    bool init() override { return true; }
};
class EmulatedKernel final : public Kernel {
public:
    EmulatedKernel() = default;
    bool init() override { return true; }
};
class CustomKernel final : public Kernel {
public:
    explicit CustomKernel(std::string const& kernel_img_path) : m_kernel_img(kernel_img_path) {}
    ~CustomKernel() override = default;
    bool init() override { return true; }

private:
    ELF m_kernel_img;
};


}