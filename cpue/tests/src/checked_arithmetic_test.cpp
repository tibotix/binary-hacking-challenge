#include <catch2/catch_all.hpp>

#include "checked_arithmetic.h"

using namespace CPUE;

TEST_CASE("CPUE_checked_single_uadd", "[checked_arithmetic]") {
    auto res = CPUE_checked_single_uadd<u8, u8>(0b00000000, 0b01111111);
    REQUIRE(res.has_cf_set == false);
    REQUIRE(res.has_of_set == false);
    res = CPUE_checked_single_uadd<u8, u8>(0b00000000, 0b11111111);
    REQUIRE(res.has_cf_set == false);
    REQUIRE(res.has_of_set == false);
    res = CPUE_checked_single_uadd<u8, u8>(0b01111111, 0b01111111);
    REQUIRE(res.has_cf_set == false);
    REQUIRE(res.has_of_set == true);
    res = CPUE_checked_single_uadd<u8, u8>(0b10000000, 0b10000000);
    REQUIRE(res.has_cf_set == true);
    REQUIRE(res.has_of_set == true);
    res = CPUE_checked_single_uadd<u8, u8>(0b10000000, 0b10000000);
    REQUIRE(res.has_cf_set == true);
    REQUIRE(res.has_of_set == true);

    res = CPUE_checked_single_uadd<u8, u8>(127, 127);
    REQUIRE(res.has_cf_set == false);
    REQUIRE(res.has_of_set == true);
    res = CPUE_checked_single_uadd<u8, u8>(127, 128);
    REQUIRE(res.has_cf_set == false); // only when 128 + 128
    REQUIRE(res.has_of_set == false);
    res = CPUE_checked_single_uadd<u8, u8>(127, 100);
    REQUIRE(res.has_cf_set == false);
    REQUIRE(res.has_of_set == true);
    res = CPUE_checked_single_uadd<u8, u8>(255, 255);
    REQUIRE(res.has_cf_set == true);
    REQUIRE(res.has_of_set == false);
    res = CPUE_checked_single_uadd<u8, u8>(255, 100);
    REQUIRE(res.has_cf_set == true);
    REQUIRE(res.has_of_set == false);
    res = CPUE_checked_single_uadd<u8, u8>(127, 130);
    REQUIRE(res.has_cf_set == true);
    REQUIRE(res.has_of_set == false);
}


TEST_CASE("CPUE_checked_single_umul", "[checked_arithmetic]") {
    auto prod = CPUE_checked_single_umul<u8, u8>(127, 2);
    REQUIRE(prod.has_cf_set == false);
    REQUIRE(prod.has_of_set == true);

    prod = CPUE_checked_single_umul<u8, u8>(127, 10);
    REQUIRE(prod.has_cf_set == true);
    REQUIRE(prod.has_of_set == true);

    prod = CPUE_checked_single_umul<u8, u8>(1, 2);
    REQUIRE(prod.has_cf_set == false);
    REQUIRE(prod.has_of_set == false);
}

TEST_CASE("CPUE_checked_single_usub", "[checked_arithmetic]") {
    auto res = CPUE_checked_single_usub<u8, u8>(0b00000001, 0b01111111);
    REQUIRE(res.has_cf_set == true);
    REQUIRE(res.has_of_set == false);

    res = CPUE_checked_single_usub<u8, u8>(0, 1);
    REQUIRE(res.has_cf_set == true);
    REQUIRE(res.has_of_set == false);

    res = CPUE_checked_single_usub<u8, u8>(0b0000001, 0b0000010);
    REQUIRE(res.has_cf_set == true);
    REQUIRE(res.has_of_set == false);
}

