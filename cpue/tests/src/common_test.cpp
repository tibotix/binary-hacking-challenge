#include <catch2/catch_all.hpp>


#include "common.h"

using namespace CPUE;

TEST_CASE("bits", "[common]") {
    REQUIRE(bits(0b0000, 0, 0) == 0b0);
    REQUIRE(bits(0b1111, 0, 0) == 0b1);
    REQUIRE(bits(0b0000, 1, 0) == 0b0);
    REQUIRE(bits(0b1111, 1, 0) == 0b11);
    REQUIRE(bits(0b1111, 1, 0) == 0b11);
    REQUIRE(bits(0b0101, 3, 0) == 0b0101);
    REQUIRE(bits(0b0101, 6, 0) == 0b0101);
    REQUIRE(bits(0b1010, 2, 1) == 0b01);
    REQUIRE(bits(0b1010, 2, 2) == 0b0);
    REQUIRE(bits(0b0110, 2, 1) == 0b11);
    REQUIRE(bits(0b0110, 3, 3) == 0b0);
    REQUIRE(bits(0b1000, 6, 3) == 0b1);
    REQUIRE(bits(0b1000, 6, 4) == 0b0);
}
