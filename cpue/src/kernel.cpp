#include "kernel.h"


namespace CPUE {


bool NoKernel::init(CPU& cpu, u64& top) {
    // The user binary will run as the "kernel" in supervisor mode, so there is nothing to do here.
    return true;
};
void NoKernel::start(CPU& cpu, u64 user_binary_entry_point) {
    CPUE_ASSERT(!cpu.handle_STI({}).raised(), "exception in STI.");

    // begin execution at user_binary_entry_point
    cpu.m_rip_val = user_binary_entry_point;
};


bool EmulatedKernel::init(CPU& cpu, u64& top) {
    // NOTE: UEFI has already set up Userspace code/data descriptors in GDT.
    TODO_NOFAIL("Configure TSS; Add TSS Descriptor in GDT");
    TODO_NOFAIL("Setup interrupt hooks");
    return true;
};
void EmulatedKernel::start(CPU& cpu, u64 user_binary_entry_point) {
    // set DS/SS/CS to Userspace segment descriptors
    TODO_NOFAIL("Use IRET to switch to ring3");
    // CPUE_ASSERT(!cpu.load_segment_register(SegmentRegisterAlias::DS, SegmentSelector(4 << 3)).raised(), "exception while loading DS.");
    // CPUE_ASSERT(!cpu.load_segment_register(SegmentRegisterAlias::SS, SegmentSelector(4 << 3)).raised(), "exception while loading SS.");
    // CPUE_ASSERT(!cpu.load_segment_register(SegmentRegisterAlias::CS, SegmentSelector(3 << 3)).raised(), "exception while loading CS.");

    // NOTE: UEFI has already set up a small stack for us and populated rsp with it.
    //       For simplicity, we use this stack for now.

    CPUE_ASSERT(!cpu.handle_STI({}).raised(), "exception in STI.");

    // begin execution at user_binary_entry_point
    cpu.m_rip_val = user_binary_entry_point;
};


bool CustomKernel::init(CPU& cpu, u64& top) {
    Loader loader{cpu};
    loader.load_supervisor_elf(&m_kernel_img, top);
    return true;
};
void CustomKernel::start(CPU& cpu, u64 user_binary_entry_point) {
    // we store the user_binary_entry_point in the first argument (rdi register)
    CPUE_ASSERT(!cpu.reg(X86_REG_RDI)->write(SizedValue(user_binary_entry_point)).raised(), "Error while writing to RDI the user_binary_entry_point.");

    // begin execution at kernel entry point
    cpu.m_rip_val = m_kernel_img.entry_point();
};


}