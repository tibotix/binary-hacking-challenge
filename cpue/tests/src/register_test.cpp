#include <catch2/catch_all.hpp>


#include "common.h"
#include "register.h"

using namespace CPUE;

TEST_CASE("read", "[register]") {
    u64 rax_val = 0xff00ff0000ff00ff;

    auto rax = RegisterFactory::QWORD(&rax_val);
    auto eax = RegisterFactory::DWORD(&rax_val);
    auto ax = RegisterFactory::WORD(&rax_val);
    auto ah = RegisterFactory::HIGH(&rax_val);
    auto al = RegisterFactory::LOW(&rax_val);

    REQUIRE(rax.read() == 0xff00ff0000ff00ff);
    REQUIRE(eax.read() == 0x00ff00ff);
    REQUIRE(ax.read() == 0x00ff);
    REQUIRE(ah.read() == 0x00);
    REQUIRE(al.read() == 0xff);
}


TEST_CASE("write", "[register]") {
    u64 rax_val = 0xff00ff0000ff00ff;

    auto rax = RegisterFactory::QWORD(&rax_val);
    auto eax = RegisterFactory::DWORD(&rax_val);
    auto ax = RegisterFactory::WORD(&rax_val);
    auto ah = RegisterFactory::HIGH(&rax_val);
    auto al = RegisterFactory::LOW(&rax_val);

    REQUIRE(rax.read() == 0xff00ff0000ff00ff);
    rax.write(0x00ff00ffff00ff00);
    REQUIRE(rax.read() == 0x00ff00ffff00ff00);
    eax.write(0x00ff00ff);
    REQUIRE(rax.read() == 0x0000000000ff00ff);
    REQUIRE(eax.read() == 0x00ff00ff);
    ax.write(0xffff);
    REQUIRE(eax.read() == 0x00ffffff);
    REQUIRE(ax.read() == 0xffff);
    REQUIRE(ah.read() == 0xff);
    REQUIRE(al.read() == 0xff);
    ah.write(0x00);
    REQUIRE(eax.read() == 0x00ff00ff);
    REQUIRE(ax.read() == 0x00ff);
    REQUIRE(ah.read() == 0x00);
    REQUIRE(al.read() == 0xff);
    al.write(0x00);
    REQUIRE(eax.read() == 0x00ff0000);
    REQUIRE(ax.read() == 0x0000);
    REQUIRE(ah.read() == 0x00);
    REQUIRE(al.read() == 0x00);
}
