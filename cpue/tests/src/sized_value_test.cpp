#include <catch2/catch_all.hpp>


#include "common.h"
#include "sized_value.h"

using namespace CPUE;

TEST_CASE("byte_width", "[sized_value]") {
    REQUIRE(SizedValue(static_cast<u8>(0)).byte_width() == ByteWidth::WIDTH_BYTE);
    REQUIRE(SizedValue(static_cast<u16>(0)).byte_width() == ByteWidth::WIDTH_WORD);
    REQUIRE(SizedValue(static_cast<u32>(0)).byte_width() == ByteWidth::WIDTH_DWORD);
    REQUIRE(SizedValue(static_cast<u64>(0)).byte_width() == ByteWidth::WIDTH_QWORD);
}

TEST_CASE("max_val", "[sized_value]") {
    REQUIRE(SizedValue(static_cast<u8>(0)).max_val() == 0xff);
    REQUIRE(SizedValue(static_cast<u16>(0)).max_val() == 0xffff);
    REQUIRE(SizedValue(static_cast<u32>(0)).max_val() == 0xffffffff);
    REQUIRE(SizedValue(static_cast<u64>(0)).max_val() == 0xffffffffffffffff);
}

TEST_CASE("sign_bit", "[sized_value]") {
    REQUIRE(SizedValue(static_cast<u8>(0)).sign_bit() == 0);
    REQUIRE(SizedValue(static_cast<u8>(1)).sign_bit() == 0);
    REQUIRE(SizedValue(static_cast<u8>(0b01111111)).sign_bit() == 0);
    REQUIRE(SizedValue(static_cast<u8>(-1)).sign_bit() == 1);
    REQUIRE(SizedValue(static_cast<u16>(0b1000000010101010)).sign_bit() == 1);
    REQUIRE(SizedValue(static_cast<u16>(0b0000000010101010)).sign_bit() == 0);
    REQUIRE(SizedValue(static_cast<u16>(-1)).sign_bit() == 1);
    REQUIRE(SizedValue(static_cast<u32>(-1)).sign_bit() == 1);
    REQUIRE(SizedValue(static_cast<u64>(-1)).sign_bit() == 1);
    REQUIRE(SizedValue(static_cast<u64>(0b111111111111111111111111)).sign_bit() == 0);
}

TEST_CASE("arithmetic", "[sized_value]") {
    REQUIRE(SizedValue(static_cast<u8>(0x1)) + SizedValue(static_cast<u8>(0xff)) == 0);
    REQUIRE(SizedValue(static_cast<u8>(0x80)) + SizedValue(static_cast<u16>(0xff)) == 0x7f);
    REQUIRE(SizedValue(static_cast<u16>(0x80)) + SizedValue(static_cast<u8>(0xff)) == 0x17f);
    REQUIRE(SizedValue(static_cast<u8>(0x1)) - SizedValue(static_cast<u8>(0x2)) == 0xff);
    REQUIRE(SizedValue(static_cast<u8>(0x1)) - SizedValue(static_cast<u16>(0x2)) == 0xff);
    REQUIRE(SizedValue(static_cast<u16>(0x1)) - SizedValue(static_cast<u8>(0x2)) == 0xffff);
}
