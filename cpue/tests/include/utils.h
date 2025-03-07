#pragma once

#include <capstone/capstone.h>
#include <string>
#include "common.h"

using namespace CPUE;

// NOTE: this functions assumes we are using no multi-threading
// TODO: check if catch2 uses multithreading or use synchronization primitive
template<typename T>
T const& make_const_ref(T t) {
    static T temp = t;
    temp = t;
    return temp;
}

#define ASM_DISAS(code) asm_disassemble(code, sizeof(code) - 1)

static cs_insn const asm_disassemble(char const* code, u64 len) {
    // TODO: Fix this for BUILD_TYPE=Release
    CPUE_ASSERT(len > 0, "empty string");
    csh handle;
    cs_insn* insn;

    CPUE_ASSERT(cs_open(CS_ARCH_X86, CS_MODE_64, &handle) == CS_ERR_OK, "testing_utils: cs_open() failed");
    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);
    size_t count = cs_disasm(handle, (u8 const*)code, len, 0x1000, 1, &insn);
    CPUE_ASSERT(count > 0, "testing utils: cs_disasm() failed");
    // TODO: make this better without leaks
    //	cs_free(insn, count);
    //	cs_close(&handle);

    return *insn;
}
