#pragma once
#include "elf.h"
#include "uefi.h"
#include "cpu.h"


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

    virtual bool init(CPU& cpu, u64& top) = 0;
    virtual void start(CPU& cpu, u64 user_binary_entry_point) = 0;
};

class NoKernel final : public Kernel {
public:
    NoKernel() = default;
    bool init(CPU& cpu, u64& top) override;
    void start(CPU& cpu, u64 user_binary_entry_point) override;
};
class EmulatedKernel final : public Kernel {
public:
    EmulatedKernel() = default;
    bool init(CPU& cpu, u64& top) override;
    void start(CPU& cpu, u64 user_binary_entry_point) override;
};
class CustomKernel final : public Kernel {
public:
    explicit CustomKernel(std::string const& kernel_img_path) : m_kernel_img(kernel_img_path) {}
    ~CustomKernel() override = default;
    bool init(CPU& cpu, u64& top) override;
    void start(CPU& cpu, u64 user_binary_entry_point) override;

private:
    ELF m_kernel_img;
};


}