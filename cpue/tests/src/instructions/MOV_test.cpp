#include <catch2/catch_all.hpp>

#include <capstone/x86.h>

#include "cpu.h"
#include "utils.h"
#include "logging.h"

using namespace CPUE;

TEST_CASE("MOV", "[instructions]") {
    CPU cpu;
    REQUIRE(cpu.reg(X86_REG_RAX)->read().release_value() == 0x0);

    auto insn = ASM_DISAS("\x66\xB8\x12\x00"); // MOV ax, 0x12
    CPUE_INFO("{} {}", insn.mnemonic, insn.op_str);
    REQUIRE(cpu.handle_insn(insn).raised() == false);
    REQUIRE(cpu.reg(X86_REG_RAX)->read().release_value() == 0x12);

    insn = ASM_DISAS("\x48\xB8\x88\x77\x66\x55\x44\x33\x22\x11"); // MOV rax, 0x1122334455667788
    CPUE_INFO("{} {}", insn.mnemonic, insn.op_str);
    REQUIRE(cpu.handle_insn(insn).raised() == false);
    REQUIRE(cpu.reg(X86_REG_RAX)->read().release_value() == 0x1122334455667788);

    insn = ASM_DISAS("\xB8\x88\x77\x66\x55"); // MOV eax, 0x0000000055667788
    CPUE_INFO("{} {}", insn.mnemonic, insn.op_str);
    REQUIRE(cpu.handle_insn(insn).raised() == false);
    REQUIRE(cpu.reg(X86_REG_RAX)->read().release_value() == 0x55667788);

    insn = ASM_DISAS("\xBB\xAD\xDE\x00\x00"); // MOV ebx, 0xdead
    CPUE_INFO("{} {}", insn.mnemonic, insn.op_str);
    REQUIRE(cpu.handle_insn(insn).raised() == false);
    REQUIRE(cpu.reg(X86_REG_RBX)->read().release_value() == 0xdead);

    insn = ASM_DISAS("\x89\xD8"); // MOV eax, ebx
    CPUE_INFO("{} {}", insn.mnemonic, insn.op_str);
    REQUIRE(cpu.handle_insn(insn).raised() == false);
    REQUIRE(cpu.reg(X86_REG_RAX)->read().release_value() == 0xdead);
}
