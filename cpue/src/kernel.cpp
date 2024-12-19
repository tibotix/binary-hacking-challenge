#include "kernel.h"


namespace CPUE {


bool NoKernel::init(u64& top) {
    // TODO: does user-elf run in supervisor or user mode here.
    return true;
};


bool EmulatedKernel::init(u64& top) {
    TODO_NOFAIL("Setup Userspace Code/Data Segment in GDT");
    TODO_NOFAIL("Configure TSS; Add TSS Descriptor in GDT");
    TODO_NOFAIL("Setup interrupt hooks");
    return true;
};

bool CustomKernel::init(u64& top) {
    if (!m_kernel_img.load())
        return false;
    TODO_NOFAIL("Load kernel-img");
    TODO_NOFAIL("set m_rip to entry point of kernel-img");
    return true;
};


}