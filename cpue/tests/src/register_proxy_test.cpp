#include <catch2/catch_all.hpp>


#include "common.h"
#include "register_proxy.h"

using namespace CPUE;

TEST_CASE("read", "[GeneralPurposeRegisterProxy]") {
    u64 rax_val = 0xff00ff0000ff00ff;

    auto rax = GeneralPurposeRegisterProxy::QWORD(&rax_val, nullptr);
    auto eax = GeneralPurposeRegisterProxy::DWORD(&rax_val, nullptr);
    auto ax = GeneralPurposeRegisterProxy::WORD(&rax_val, nullptr);
    auto ah = GeneralPurposeRegisterProxy::HIGH(&rax_val, nullptr);
    auto al = GeneralPurposeRegisterProxy::LOW(&rax_val, nullptr);

    REQUIRE(rax.read().release_value() == 0xff00ff0000ff00ff);
    REQUIRE(eax.read().release_value() == 0x00ff00ff);
    REQUIRE(ax.read().release_value() == 0x00ff);
    REQUIRE(ah.read().release_value() == 0x00);
    REQUIRE(al.read().release_value() == 0xff);
}


TEST_CASE("write", "[GeneralPurposeRegisterProxy]") {
    u64 rax_val = 0xff00ff0000ff00ff;

    auto rax = GeneralPurposeRegisterProxy::QWORD(&rax_val, nullptr);
    auto eax = GeneralPurposeRegisterProxy::DWORD(&rax_val, nullptr);
    auto ax = GeneralPurposeRegisterProxy::WORD(&rax_val, nullptr);
    auto ah = GeneralPurposeRegisterProxy::HIGH(&rax_val, nullptr);
    auto al = GeneralPurposeRegisterProxy::LOW(&rax_val, nullptr);

    REQUIRE(rax.read().release_value() == 0xff00ff0000ff00ff);
    REQUIRE(rax.write(SizedValue(static_cast<u64>(0x00ff00ffff00ff00))).raised() == false);
    REQUIRE(rax.read().release_value() == 0x00ff00ffff00ff00);
    REQUIRE(eax.write(SizedValue(static_cast<u32>(0x00ff00ff))).raised() == false);
    REQUIRE(rax.read().release_value() == 0x0000000000ff00ff);
    REQUIRE(eax.read().release_value() == 0x00ff00ff);
    REQUIRE(ax.write(SizedValue(static_cast<u16>(0xffff))).raised() == false);
    REQUIRE(eax.read().release_value() == 0x00ffffff);
    REQUIRE(ax.read().release_value() == 0xffff);
    REQUIRE(ah.read().release_value() == 0xff);
    REQUIRE(al.read().release_value() == 0xff);
    REQUIRE(ah.write(SizedValue(static_cast<u8>(0x00))).raised() == false);
    REQUIRE(eax.read().release_value() == 0x00ff00ff);
    REQUIRE(ax.read().release_value() == 0x00ff);
    REQUIRE(ah.read().release_value() == 0x00);
    REQUIRE(al.read().release_value() == 0xff);
    REQUIRE(al.write(SizedValue(static_cast<u8>(0x00))).raised() == false);
    REQUIRE(eax.read().release_value() == 0x00ff0000);
    REQUIRE(ax.read().release_value() == 0x0000);
    REQUIRE(ah.read().release_value() == 0x00);
    REQUIRE(al.read().release_value() == 0x00);
}
