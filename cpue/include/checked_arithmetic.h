#pragma once

#include "common.h"
#include "sized_value.h"


namespace CPUE {

struct ArithmeticResult {
    SizedValue value;
    bool has_of_set;
    bool has_cf_set;
    // bool has_sf_set;
    // bool has_zf_set;
    /* Is used for BCD-Type arithmetic,
     * which we don't include
     * bool has_af_set;
    */

    constexpr ArithmeticResult() : has_of_set(false), has_cf_set(false) {}
};

// NOTE: SizedValue is a helper struct which include multiple small functions to gain details about the operands
constexpr ArithmeticResult CPUE_checked_single_uadd(SizedValue const& first, SizedValue const& summand) {
    ArithmeticResult res;
    res.value = first;

    u64 MAX_VAL = first.max_val();

    bool first_sign_bit = first.sign_bit();
    bool summand_sign_bit = summand.sign_bit();

    // Set Carry-Flag if including this summand would exceed MAX_VAL
    size_t max_summand = MAX_VAL - res.value.value();
    if (summand > max_summand)
        res.has_cf_set = true;
    res.value += summand;

    // Setting OVERFLOW-BIT
    if (first_sign_bit == summand_sign_bit && first_sign_bit != res.value.sign_bit())
        res.has_of_set = true;

    return res;
}

template<unsigned_integral R, typename T>
requires(std::is_same_v<R, T>) constexpr ArithmeticResult CPUE_checked_single_uadd(R const first, T const summand) {
    return CPUE_checked_single_uadd(SizedValue(first), SizedValue(summand));
}

// Not allowing overflows
template<unsigned_integral R, typename... T>
requires(std::is_same_v<R, T>&&...) constexpr R CPUE_checked_uadd(R const first, T const... factors) {
    // Consider calculation invalid if we are only adding one number
    static_assert(sizeof...(factors) > 0, "You have to add at least two numbers together.");
    R res = first;

    for (const auto f : {factors...}) {
        // Fail if including this additional factor would overflow (set carry flag in unsigned arithmetic)
        auto r = CPUE_checked_single_uadd(res, f);
        if (r.has_cf_set)
            fail("Integer overflow in addition.");
        res = r.value.template as<R>();
    }

    return res;
}

// NOTE:
// The OF and CF flags are set to 0 if the upper half of the result is 0; otherwise, they are set to 1.
// The SF, ZF, AF, and PF flags are undefined.
constexpr ArithmeticResult CPUE_checked_single_umul(SizedValue const& first, SizedValue const& summand) {
    ArithmeticResult res;
    uint64_t product = first.value() * summand.value();

    uint64_t maxValue = first.max_val();
    uint64_t highBits = product >> first.bit_width();

    // Set Carry Flag (CF) - if High-order bits are non-zero indicates unsigned overflow
    res.has_cf_set = (highBits != 0);

    // Set Overflow Flag (OF) - Signed overflow if the signs of inputs match but differ from the result
    bool sign_first = first.sign_bit();
    bool sign_second = summand.sign_bit();
    bool sign_result = ((product >> (first.bit_width() - 1)) & 1);

    res.has_of_set = (sign_first == sign_second) && (sign_first != sign_result);

    //NOTE: vorschlag von chat
    /*res.value = SizedValue(product, first.bit_width()); // Ensure the result fits into the same operand size*/

    res.value = product;

    return res;
}

template<unsigned_integral R, typename T>
requires(std::is_same_v<R, T>) constexpr ArithmeticResult CPUE_checked_single_umul(R const first, T const summand) {
    return CPUE_checked_single_umul(SizedValue(first), SizedValue(summand));
}

template<unsigned_integral R, typename... T>
requires(std::is_same_v<R, T>&&...) constexpr R CPUE_checked_umul(R const first, T const... factors) {
    // Consider calculation invalid if we are only multiplying one number
    static_assert(sizeof...(factors) > 0, "You have to multiply at least two numbers together.");

    R res = first;

    for (const auto f : {factors...}) {
        auto r = CPUE_checked_single_umul(res, f);
        if (r.has_cf_set)
            fail("Integer overflow in multiplikation.");
        res = r.value.template as<R>();
    }

    return res;
}

// NOTE:
// Two memory operands cannot be used in one instruction.
// OF and CF flags to indicate an overflow in the signed or unsigned result, respectively.
// The SF flag indicates the sign of the signed result
// OF, SF, ZF, and CF flags
constexpr ArithmeticResult CPUE_checked_single_usub(SizedValue const& first, SizedValue const& summand) {
    ArithmeticResult res;

    res.value = first.value() - summand.value();

    if (summand.value() > first.value())
        res.has_cf_set = true;

    // Setting OVERFLOW-BIT
    if (first.sign_bit() != summand.sign_bit() && first.sign_bit() != res.value.sign_bit())
        res.has_of_set = true;

    return res;
}

template<unsigned_integral R, typename T>
requires(std::is_same_v<R, T>) constexpr ArithmeticResult CPUE_checked_single_usub(R const first, T const summand) {
    return CPUE_checked_single_usub(SizedValue(first), SizedValue(summand));
}

template<unsigned_integral R, typename... T>
requires(std::is_same_v<R, T>&&...) constexpr R CPUE_checked_usub(R const first, T const... factors) {
    // Consider calculation invalid if we are only multiplying one number
    static_assert(sizeof...(factors) > 0, "You have to subtract at least two numbers together.");

    R res = first;

    for (const auto f : {factors...}) {
        auto r = CPUE_checked_single_usub(res, f);
        if (r.has_cf_set)
            fail("Integer overflow in subtraction.");
        res = r.value.template as<R>();
    }

    return res;
}

constexpr ArithmeticResult CPUE_checked_single_udiv(SizedValue const& first, SizedValue const& summand) {
    TODO("Not yet implemented!");
    ArithmeticResult res;
    return res;
}



}
