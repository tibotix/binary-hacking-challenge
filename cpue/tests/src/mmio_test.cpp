#include <catch2/catch_all.hpp>


#include "common.h"
#include "address.h"
#include "mmio.h"

#include "utils.h"

using namespace CPUE;



TEST_CASE("make_ptr_mmio_reg", "[mmio]") {
    u32 reg = 0x01020304;
    auto mmio_reg = make_ptr_mmio_reg(&reg);
    REQUIRE(mmio_reg.width == ByteWidth::WIDTH_DWORD);
    REQUIRE(mmio_reg.read().value() == reg);
    mmio_reg.write(SizedValue(0xff020304, ByteWidth::WIDTH_DWORD));
    REQUIRE(reg == 0xff020304);
    REQUIRE(mmio_reg.read().value() == 0xff020304);
}

TEST_CASE("map_mmio_register", "[mmio]") {
    MMIO mmio;
    u32 reg = 0x01020304;
    auto mmio_reg = make_ptr_mmio_reg(&reg);
    REQUIRE(!mmio.try_mmio_read<u32>(make_const_ref(0x80_pa)).release_value().has_value());
    mmio.map_mmio_register(make_const_ref(0x80_pa), mmio_reg);
    REQUIRE(mmio.try_mmio_read<u32>(make_const_ref(0x80_pa)).release_value().has_value());
}

TEST_CASE("try_mmio_read", "[mmio]") {
    MMIO mmio;
    u32 reg = 0x01020304;
    auto mmio_reg = make_ptr_mmio_reg(&reg);
    mmio.map_mmio_register(make_const_ref(0x80_pa), mmio_reg);
    REQUIRE(mmio.try_mmio_read<u32>(make_const_ref(0x80_pa)).release_value().value().value == 0x01020304);
    reg = 0xff020304;
    REQUIRE(mmio.try_mmio_read<u32>(make_const_ref(0x80_pa)).release_value().value().value == 0xff020304);
    REQUIRE(!mmio.try_mmio_read<u32>(make_const_ref(0x70_pa)).release_value().has_value());
    REQUIRE(!mmio.try_mmio_read<u32>(make_const_ref(0x84_pa)).release_value().has_value());
}

TEST_CASE("try_mmio_write", "[mmio]") {
    MMIO mmio;
    u32 reg = 0x01020304;
    auto mmio_reg = make_ptr_mmio_reg(&reg);
    REQUIRE(!mmio.try_mmio_write(make_const_ref(0x80_pa), make_const_ref(BigEndian(0xff020304u))).release_value());
    mmio.map_mmio_register(make_const_ref(0x80_pa), mmio_reg);
    REQUIRE(mmio.try_mmio_write<u32>(make_const_ref(0x80_pa), make_const_ref(BigEndian<u32>(0xff020304u))).release_value());
    REQUIRE(reg == 0xff020304);
}
