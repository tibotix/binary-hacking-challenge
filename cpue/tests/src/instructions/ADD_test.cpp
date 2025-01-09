#include <catch2/catch_all.hpp>

#include <capstone/x86.h>

#include "cpu.h"
#include "utils.h"
#include "logging.h"

using namespace CPUE;

TEST_CASE("ADD", "[instructions]") {
    CPU cpu;
    REQUIRE(cpu.gpreg64(X86_REG_RAX).value()->read() == 0x0);

    auto insn = asm_disassemble("\x48\x83\xC0\x08"); // ADD rax, 8
    CPUE_INFO("{} {}", insn.mnemonic, insn.op_str);
    REQUIRE(cpu.handle_insn(insn).raised() == false);
    REQUIRE(cpu.gpreg64(X86_REG_RAX).value()->read() == 0x8);

    insn = asm_disassemble("\x83\xC0\x08"); // ADD eax, 8
    CPUE_INFO("{} {}", insn.mnemonic, insn.op_str);
    REQUIRE(cpu.handle_insn(insn).raised() == false);
    REQUIRE(cpu.gpreg64(X86_REG_RAX).value()->read() == 0x10);
}
