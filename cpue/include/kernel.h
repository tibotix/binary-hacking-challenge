#pragma once
#include "elf.h"
#include "uefi.h"


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

    virtual bool init(u64& top) = 0;
};

class NoKernel final : public Kernel {
public:
    NoKernel() = default;
    bool init(u64& top) override;
};
class EmulatedKernel final : public Kernel {
public:
    EmulatedKernel() = default;
    bool init(u64& top) override;
};
class CustomKernel final : public Kernel {
public:
    explicit CustomKernel(std::string const& kernel_img_path) : m_kernel_img(kernel_img_path) {}
    ~CustomKernel() override = default;
    bool init(u64& top) override;

private:
    ELF m_kernel_img;
};


}