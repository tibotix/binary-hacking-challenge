#pragma once

#include <limits>
#include "common.h"


namespace CPUE {

template<typename R>
struct SumBits {
    R value;
    bool has_of_set;
    bool has_cf_set;

    constexpr SumBits() : has_of_set(false), has_cf_set(false) {}
};


template<typename R, typename T>
requires(std::is_same_v<R, T>) && (std::is_integral_v<R> && std::is_unsigned_v<R>)constexpr SumBits<R> CPUE_checked_single_uadd(R const first, T const summand) {
    SumBits<R> res;
    res.value = first;

    constexpr R MAX_VAL = std::numeric_limits<R>::max();

    bool first_sign_bit = first >> (sizeof(R) * 8 - 1);
    bool summand_sign_bit = summand >> (sizeof(T) * 8 - 1);

    // Set Carry-Flag if including this summand would exceed MAX_VAL
    size_t max_summand = MAX_VAL - res.value;
    if (summand > max_summand) {
        res.has_cf_set = true;
    }
    res.value += summand;

    bool res_sign_bit = res.value >> (sizeof(R) * 8 - 1);
    if (first_sign_bit == summand_sign_bit && first_sign_bit != res_sign_bit) {
        res.has_of_set = true;
    }

    return res;
}

// Not allowing overflows
template<typename R, typename... T>
requires(std::is_same_v<R, T>&&...) && (std::is_integral_v<R> && std::is_unsigned_v<R>)constexpr R CPUE_checked_uadd(R const first, T const... factors) {
    // Consider calculation invalid if we are only adding one number
    static_assert(sizeof...(factors) > 0, "You have to add at least two numbers together.");
    R res = first;

    for (const auto f : {factors...}) {
        // Fail if including this additional factor would overflow (set carry flag in unsigned arithmetic)
        auto r = CPUE_checked_single_uadd(res, f);
        if (r.has_cf_set)
            fail("Integer overflow in addition.");
        res = r.value;
    }

    return res;
}


template<typename R, typename... T>
requires(std::is_same_v<R, T>&&...) && (std::is_integral_v<R> && std::is_unsigned_v<R>)constexpr R CPUE_checked_umul(R const first, T const... factors) {
    // Consider calculation invalid if we are only multiplying one number
    static_assert(sizeof...(factors) > 0, "You have to multiply at least two numbers together.");

    constexpr R MAX_VAL = std::numeric_limits<R>::max();

    int i = 0;
    R res = first;

    for (const auto f : {factors...}) {
        // Fail if including this additional factor would exceed MAX_VAL
        size_t max_factor = MAX_VAL / res;
        if (f > max_factor)
            fail("Integer overflow in multiplication.");
        res *= f;
    }
    return res;
}


}
