#include <catch2/catch_all.hpp>

#include <capstone/x86.h>

#include "cpu.h"
#include "utils.h"
#include "logging.h"

using namespace CPUE;

TEST_CASE("ADD", "[instructions]") {
    CPU cpu;
    REQUIRE(cpu.reg(X86_REG_RAX)->read().release_value() == 0x0);

    auto insn = ASM_DISAS("\x48\x83\xC0\x08"); // ADD rax, 8
    CPUE_INFO("{} {}", insn.mnemonic, insn.op_str);
    REQUIRE(cpu.handle_insn(insn).raised() == false);
    REQUIRE(cpu.reg(X86_REG_RAX)->read().release_value() == 0x8);
    REQUIRE(cpu.rflags().c.ZF == false);

    insn = ASM_DISAS("\x83\xC0\x08"); // ADD eax, 8
    CPUE_INFO("{} {}", insn.mnemonic, insn.op_str);
    REQUIRE(cpu.handle_insn(insn).raised() == false);
    REQUIRE(cpu.reg(X86_REG_RAX)->read().release_value() == 0x10);
    REQUIRE(cpu.rflags().c.ZF == false);

    insn = ASM_DISAS("\x04\xF0"); // ADD al, 0xf0
    CPUE_INFO("{} {}", insn.mnemonic, insn.op_str);
    REQUIRE(cpu.handle_insn(insn).raised() == false);
    REQUIRE(cpu.reg(X86_REG_RAX)->read().release_value() == 0x0);
    REQUIRE(cpu.rflags().c.ZF == true);
    REQUIRE(cpu.rflags().c.CF == true);
    REQUIRE(cpu.rflags().c.OF == false);
}
