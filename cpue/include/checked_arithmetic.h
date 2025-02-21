#pragma once

#include "common.h"
#include "sized_value.h"


namespace CPUE {

struct ArithmeticResult {
    SizedValue value;
    bool has_of_set;
    bool has_cf_set;
    /* Is used for BCD-Type arithmetic,
     * which we don't include
     * bool has_af_set;
    */

    constexpr ArithmeticResult() : value(0), has_of_set(false), has_cf_set(false){};
    constexpr ArithmeticResult(SizedValue const& value, bool has_of_set = false, bool has_cf_set = false) : value(value), has_of_set(has_of_set), has_cf_set(has_cf_set) {}
};

constexpr ArithmeticResult CPUE_checked_single_uadd(SizedValue const& first, SizedValue const& summand) {
    ArithmeticResult res{first};

    u128 MAX_VAL = first.max_val();

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
requires(std::is_same_v<R, T>&&...) constexpr R CPUE_checked_uadd(R const first, T const... summands) {
    // Consider calculation invalid if we are only adding one number
    static_assert(sizeof...(summands) > 0, "You have to add at least two numbers together.");
    R res = first;

    for (const auto s : {summands...}) {
        // Fail if including this additional factor would overflow (set carry flag in unsigned arithmetic)
        auto r = CPUE_checked_single_uadd(res, s);
        if (r.has_cf_set)
            fail("Integer overflow in addition.");
        res = r.value.template as<R>();
    }

    return res;
}

constexpr ArithmeticResult CPUE_checked_single_umul(SizedValue const& first, SizedValue const& factor) {
    u128 result = first.value() * factor.value();

    // construct value twice the size of first value
    SizedValue value = SizedValue(result, first.byte_width().double_width());
    // The OF and CF flags are set to 0 if the upper half of the result is 0; otherwise, they are set to 1.
    bool upper_half_not_zero = value.upper_half().value() != 0;
    return {value, upper_half_not_zero, upper_half_not_zero};
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
            fail("Integer overflow in multiplication.");
        res = r.value.template as<R>();
    }

    return res;
}


constexpr ArithmeticResult CPUE_checked_single_imul(SizedValue const& first, SizedValue const& factor) {
    i128 a = first.sign_extended_to_width(ByteWidth::WIDTH_DQWORD).value();
    i128 b = factor.sign_extended_to_width(ByteWidth::WIDTH_DQWORD).value();

    u128 result = a * b;

    // construct value twice the size of first value
    SizedValue value = SizedValue(result, first.byte_width().double_width());
    // The CF and OF flags are set when the signed integer value of the intermediate product differs from the sign extended
    // operand-size-truncated product, otherwise the CF and OF flags are cleared.
    auto truncated = SizedValue(result, first.byte_width()).sign_extended_to_width(value.byte_width());
    bool truncated_value_differs_from_infinite = value != truncated;
    return {value, truncated_value_differs_from_infinite, truncated_value_differs_from_infinite};
}

template<signed_integral R, typename T>
requires(std::is_same_v<R, T>) constexpr ArithmeticResult CPUE_checked_single_imul(R const first, T const summand) {
    return CPUE_checked_single_imul(SizedValue(first), SizedValue(summand));
}

template<signed_integral R, typename... T>
requires(std::is_same_v<R, T>&&...) constexpr R CPUE_checked_imul(R const first, T const... factors) {
    // Consider calculation invalid if we are only multiplying one number
    static_assert(sizeof...(factors) > 0, "You have to multiply at least two numbers together.");

    R res = first;

    for (const auto f : {factors...}) {
        auto r = CPUE_checked_single_imul(res, f);
        if (r.has_cf_set)
            fail("Integer overflow in multiplication.");
        res = r.value.template as<R>();
    }

    return res;
}


constexpr ArithmeticResult CPUE_checked_single_usub(SizedValue const& first, SizedValue const& summand) {
    auto result = first - summand;
    ArithmeticResult res{result};

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



}
